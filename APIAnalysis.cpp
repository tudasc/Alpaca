#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/FrontendActions.h>
#include "clang/Tooling/Tooling.h"
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/CommandLine.h>

#include "include/cxxopts.hpp"

#include "header/HelperFunctions.h"
#include "header/Analyser.h"
#include "filesystem"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace clang;
using namespace helper;
using namespace analyse;

template<typename T, class... Args>
std::unique_ptr<clang::tooling::FrontendActionFactory> argumentParsingFrontendActionFactory(Args... args){

    class SimpleFrontendActionFactory : public clang::tooling::FrontendActionFactory {
    public:
        SimpleFrontendActionFactory(Args... constructorArgs) {
            fn1=[=](){ return std::make_unique<T>(constructorArgs...);};
        }

        std::unique_ptr<clang::FrontendAction> create() override {
            return fn1();
        }

    private:
        std::function<std::unique_ptr<T>()> fn1; // function
    };

    return std::unique_ptr<FrontendActionFactory>(new SimpleFrontendActionFactory(args...));
}


class APIAnalysisVisitor : public clang::RecursiveASTVisitor<APIAnalysisVisitor> {
    std::multimap<std::string, FunctionInstance>* program;
    std::string dir;
public:
    explicit APIAnalysisVisitor(clang::ASTContext *Context, std::multimap<std::string, FunctionInstance>* program, std::string directory) : Context(Context){
        this->program = program;
        this->dir = directory;
    }

    bool VisitFunctionDecl(clang::FunctionDecl *functionDecl){
        // skip this function, if it's declared in the header files
        if(!Context->getSourceManager().isInMainFile(functionDecl->getLocation())){
            return true;
        }

        FunctionInstance functionInstance;

        functionInstance.name = functionDecl->getNameAsString();
        functionInstance.returnType = functionDecl->getDeclaredReturnType().getAsString();

        int numParam = functionDecl->getNumParams();
        for(int i=0;i<numParam;i++){
            functionInstance.params.push_back(functionDecl->getParamDecl(i)->getType().getAsString());
        }

        // gets a vector of all the classes / namespaces a function is part of e.g. simple::example::function() -> [simple, example]
        std::string fullName = functionDecl->getQualifiedNameAsString();
        std::string delimiter = "::";

        size_t pos = 0;
        std::string singleNamespace;
        while ((pos = fullName.find(delimiter)) != std::string::npos) {
            singleNamespace = fullName.substr(0, pos);
            functionInstance.location.push_back(singleNamespace);
            fullName.erase(0, pos + delimiter.length());
        }

        // gets the File Name
        FullSourceLoc FullLocation = Context->getFullLoc(functionDecl->getBeginLoc());
        functionInstance.filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(functionDecl->getBeginLoc()).str()), dir);

        // saves the function body as a string
        auto start = functionDecl->getBody()->getBeginLoc(), end = functionDecl->getBody()->getEndLoc();
        LangOptions lang;
        SourceManager *sm = &(Context->getSourceManager());
        auto endToken = Lexer::getLocForEndOfToken(end, 0, *sm, lang);
        functionInstance.body = std::string(sm->getCharacterData(start), sm->getCharacterData(endToken)-sm->getCharacterData(start));

        // get the scope of the functions (for some reason global functions dont have an access value, so it is set manually)
        if(getAccessSpelling(functionDecl->getAccess())==""){
            functionInstance.scope = "public";
        }else{
            functionInstance.scope = getAccessSpelling(functionDecl->getAccess());
        }

        program->insert(std::make_pair(functionInstance.name, functionInstance));
        return true;
    };

private:
    clang::ASTContext *Context;
};


class APIAnalysisConsumer : public clang::ASTConsumer {
public:
    explicit APIAnalysisConsumer(clang::ASTContext *Context, std::multimap<std::string, FunctionInstance>* program, std::string dir) : apiAnalysisVisitor(Context, program, dir){}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        apiAnalysisVisitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    APIAnalysisVisitor apiAnalysisVisitor;
};


class APIAnalysisAction : public clang::ASTFrontendAction {
    std::multimap<std::string, FunctionInstance>* p;
    std::string directory;
public:
    explicit APIAnalysisAction(std::multimap<std::string, FunctionInstance>* program, std::string dir){
        this->p=program;
        this->directory = dir;
    };

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::make_unique<APIAnalysisConsumer>(&Compiler.getASTContext(), p, directory);
    }

private:
};


int main(int argc, const char **argv) {

    cxxopts::Options options("APIAnalysis", "Compares two versions of a C++ API and prints out the differences");
    options.add_options()
            ("doc, deep-overload-comparison", "Enables the statistical comparison of function bodies")
            ("newDir, newDirectory", "Path to the newer version", cxxopts::value<std::string>())
            ("oldDir, oldDirectory", "Path to the older version", cxxopts::value<std::string>());

    auto result = options.parse(argc, argv);

    std::vector<std::string> oldFiles;
    std::vector<std::string> newFiles;

    if(result.count("oldDir")){
        listFiles(result["oldDir"].as<std::string>(), &oldFiles);
        outs()<<"The old directory contains " + itostr(oldFiles.size()) + " files\n";

    }else{
        throw std::invalid_argument("The older version of the project has to be specified");
    }

    if(result.count("newDir")){
        listFiles(result["newDir"].as<std::string>(), &newFiles);
        outs()<<"The new directory contains " + itostr(newFiles.size()) + " files\n";

    }else{
        throw std::invalid_argument("The older version of the project has to be specified");
    }
    bool docEnabled = result["doc"].as<bool>();

    std::multimap<std::string, FunctionInstance> oldProgram;
    std::multimap<std::string, FunctionInstance> newProgram;

    std::string errorMessage="The Compilationdatabase could not be found";
    auto oldCD = CompilationDatabase::loadFromDirectory(std::filesystem::canonical(result["oldDir"].as<std::string>()).string(),errorMessage);
    auto newCD = CompilationDatabase::loadFromDirectory(std::filesystem::canonical(result["newDir"].as<std::string>()).string(),errorMessage);

    // check if the compilation databases exist, otherwise use the standard provided in the build directory
    if(!oldCD){
        // put a standard empty CompilationDatabase here
        oldCD = CompilationDatabase::loadFromDirectory(std::filesystem::current_path().string(), errorMessage);
    }
    if(!newCD){
        newCD = CompilationDatabase::loadFromDirectory(std::filesystem::current_path().string(), errorMessage);
    }


    ClangTool oldTool(*oldCD,
                 oldFiles);
    oldTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&oldProgram, result["oldDir"].as<std::string>()).get());

    ClangTool newTool(*newCD,
                   newFiles);
    newTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&newProgram, result["newDir"].as<std::string>()).get());

    // Analysing
    Analyser analyser = Analyser(oldProgram, newProgram);

    analyser.compareVersions();

    outs()<<"\n";
    return 0;
}