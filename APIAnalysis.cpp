#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/FrontendActions.h>
#include "clang/Tooling/Tooling.h"
#include <clang/AST/RecursiveASTVisitor.h>
#include "clang/Tooling/ArgumentsAdjusters.h"
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

        outs()<<"Path in AST Traverser is: " + std::filesystem::current_path().string()<<"\n";

        FunctionInstance functionInstance;

        functionInstance.name = functionDecl->getNameAsString();
        functionInstance.returnType = functionDecl->getDeclaredReturnType().getAsString();

        unsigned int numParam = functionDecl->getNumParams();
        for(int i=0;i<numParam;i++){
            auto paramDecl = functionDecl->getParamDecl(i);
            std::string defaultParam;
            if(paramDecl->hasDefaultArg()){
                //defaultParam = paramDecl->getDefaultArgRange().(Context->getSourceManager());
                auto start = paramDecl->getDefaultArg()->getBeginLoc(), end = paramDecl->getDefaultArg()->getEndLoc();
                LangOptions lang;
                SourceManager *sm = &(Context->getSourceManager());
                auto endToken = Lexer::getLocForEndOfToken(end, 0, *sm, lang);
                defaultParam = std::string(sm->getCharacterData(start),
                                                    sm->getCharacterData(endToken) - sm->getCharacterData(start));

            }
            // TODO: Check if I can / should strip keywords like const or &
            functionInstance.params.push_back(std::make_pair(paramDecl->getType().getAsString(), std::make_pair(paramDecl->getNameAsString(), defaultParam)));
        }

        // gets a vector of all the classes / namespaces a function is part of e.g. simple::example::function() -> [simple, example]
        std::string fullName = functionDecl->getQualifiedNameAsString();
        std::string delimiter = "::";

        int pos = 0;
        std::string singleNamespace;
        while ((pos = fullName.find(delimiter)) != std::string::npos) {
            singleNamespace = fullName.substr(0, pos);
            functionInstance.location.push_back(singleNamespace);
            fullName.erase(0, pos + delimiter.length());
        }

        // gets the File Name
        FullSourceLoc FullLocation = Context->getFullLoc(functionDecl->getBeginLoc());
        auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(functionDecl->getBeginLoc()).str()), dir);
        functionInstance.filename = filename;

        // save the qualified name of the function
        functionInstance.qualifiedName = functionDecl->getQualifiedNameAsString();

        if(functionDecl->isThisDeclarationADefinition()){
            functionInstance.isDeclaration = false;
            // saves the function body as a string
            auto start = functionDecl->getBody()->getBeginLoc(), end = functionDecl->getBody()->getEndLoc();
            LangOptions lang;
            SourceManager *sm = &(Context->getSourceManager());
            auto endToken = Lexer::getLocForEndOfToken(end, 0, *sm, lang);
            functionInstance.body = std::string(sm->getCharacterData(start),
                                                sm->getCharacterData(endToken) - sm->getCharacterData(start));
        } else {
            // marks the function as a Declaration and doesn't save the body
            functionInstance.isDeclaration = true;
        }
        // get the scope of the functions (for some reason global functions don't have an access value, so it is set manually)
        if(getAccessSpelling(functionDecl->getAccess()).empty()){
            functionInstance.scope = "public";
        }else{
            functionInstance.scope = getAccessSpelling(functionDecl->getAccess());
        }

        program->insert(std::make_pair(functionInstance.qualifiedName, functionInstance));
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

void assignDeclarations(std::multimap<std::string, FunctionInstance>& functions){
    for (auto &item: functions){
        if(item.second.isDeclaration){
            continue;
        }
        // find all instances of this particular qualified name
        for (auto[it, end] = functions.equal_range(item.first); it != end; it++)
        {
            // if the current is a matching declaration, it is added to the FunctionItem item
            if(item.second.isCorrectDeclaration(it->second)){
                item.second.declarations.push_back(it->second);
            }
        }
    }

    // delete all the declarations from the list
    for (std::multimap<std::string, FunctionInstance>::iterator iter = functions.begin(); iter != functions.end();)
    {
        std::multimap<std::string, FunctionInstance>::iterator erase_iter = iter++;

        // removes the function if it is a declaration
        if (erase_iter->second.isDeclaration)
            functions.erase(erase_iter);
    }
}

int main(int argc, const char **argv) {
    cxxopts::Options options("APIAnalysis", "Compares two versions of a C++ API and prints out the differences");
    options.add_options()
            ("doc, deep-overload-comparison", "Enables the statistical comparison of function bodies")
            ("ipf,include-private-functions", "Include private functions in the output")
            ("json,output-JSON", "Output the analysis in a custom JSON format")
            ("newDir, newDirectory", "Path to the newer version of the project - REQUIRED", cxxopts::value<std::string>())
            ("oldDir, oldDirectory", "Path to the older version of the project - REQUIRED", cxxopts::value<std::string>())
            ("oldCD, oldCompilationDatabase", "Path to the compilation database of the old directory", cxxopts::value<std::string>())
            ("newCD, newCompilationDatabase", "Path to the compilation database of the new directory", cxxopts::value<std::string>())
            ("extra-args, extra-arguments", "Add additional clang arguments for both projects, separated with ,", cxxopts::value<std::vector<std::string>>())
            ("extra-args-old, extra-arguments-old", "Add additional clang arguments for the old project, separated with ,", cxxopts::value<std::vector<std::string>>())
            ("extra-args-new, extra-arguments-new", "Add additional clang arguments for the new project, separated with ,", cxxopts::value<std::vector<std::string>>())
            ("h,help", "Print usage")
            ;
    auto result = options.parse(argc, argv);

    if(result.count("help")){
        outs()<<options.help();
        return 0;
    }

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
        throw std::invalid_argument("The older version of the project has to be specified (--oldDir has to be set)");
    }

    bool docEnabled = result["doc"].as<bool>();

    bool outputPrivateFunctions = result["ipf"].as<bool>();

    bool jsonOutput = result["json"].as<bool>();

    std::multimap<std::string, FunctionInstance> oldProgram;
    std::multimap<std::string, FunctionInstance> newProgram;

    std::unique_ptr<CompilationDatabase> oldCD;
    std::unique_ptr<CompilationDatabase> newCD;

    if(result.count("oldCD")){
        std::string errorMessage = "Could not load the specified old compilation Database, trying to find one in the project files\n";
        oldCD = FixedCompilationDatabase::autoDetectFromDirectory(std::filesystem::canonical(result["oldCD"].as<std::string>()).string(), errorMessage);
    }

    if(result.count("newCD")){
        std::string errorMessage = "Could not load the specified new compilation Database, trying to find one in the project files\n";
        newCD = FixedCompilationDatabase::autoDetectFromDirectory(std::filesystem::canonical(result["newCD"].as<std::string>()).string(), errorMessage);
    }

    std::string errorMessage="No Compilation database could be found in the old directory, loading the standard empty compilation database";
    if(!oldCD) {
        oldCD = CompilationDatabase::autoDetectFromDirectory(
                std::filesystem::canonical(result["oldDir"].as<std::string>()).string(), errorMessage);
    }
    if(!newCD) {
        newCD = CompilationDatabase::autoDetectFromDirectory(
                std::filesystem::canonical(result["newDir"].as<std::string>()).string(), errorMessage);
    }

    // check if the compilation databases exist, otherwise use the standard provided in the build directory
    errorMessage = "Fatal error, the standard compilation database couldn't be loaded";
    if(!oldCD){
        outs()<<"loaded an empty compilation database for the old project\n";
        // put a standard empty CompilationDatabase here
        oldCD = FixedCompilationDatabase::loadFromBuffer(".","",errorMessage);
    }
    if(!newCD){
        outs()<<"loaded an empty compilation database for the new project\n";
        newCD = FixedCompilationDatabase::loadFromBuffer(".","",errorMessage);
    }

    outs()<<"Original Path is: " + std::filesystem::current_path().string()<<"\n";

    ClangTool oldTool(*oldCD,
                      oldFiles);

    if(result.count("extra-args")) {
        CommandLineArguments args = result["extra-args"].as<std::vector<std::string>>();
        auto adjuster = clang::tooling::getInsertArgumentAdjuster(args, ArgumentInsertPosition::BEGIN);
        oldTool.appendArgumentsAdjuster(adjuster);
    }

    if(result.count("extra-args-old")) {
        CommandLineArguments args = result["extra-args-old"].as<std::vector<std::string>>();
        auto adjuster = clang::tooling::getInsertArgumentAdjuster(args, ArgumentInsertPosition::BEGIN);
        oldTool.appendArgumentsAdjuster(adjuster);
    }

    oldTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&oldProgram, std::filesystem::canonical(std::filesystem::absolute(result["oldDir"].as<std::string>()))).get());

    ClangTool newTool(*newCD,
                   newFiles);

    if(result.count("extra-args")) {
        CommandLineArguments args = result["extra-args"].as<std::vector<std::string>>();
        auto adjuster = clang::tooling::getInsertArgumentAdjuster(args, ArgumentInsertPosition::BEGIN);
        newTool.appendArgumentsAdjuster(adjuster);
    }

    if(result.count("extra-args-new")) {
        CommandLineArguments args = result["extra-args-new"].as<std::vector<std::string>>();
        auto adjuster = clang::tooling::getInsertArgumentAdjuster(args, ArgumentInsertPosition::BEGIN);
        newTool.appendArgumentsAdjuster(adjuster);
    }
    newTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&newProgram, std::filesystem::canonical(std::filesystem::absolute(result["newDir"].as<std::string>()))).get());

    assignDeclarations(oldProgram);
    assignDeclarations(newProgram);

    // Analysing
    Analyser analyser = Analyser(oldProgram, newProgram, jsonOutput);

    analyser.compareVersionsWithDoc(docEnabled, outputPrivateFunctions);

    return 0;
}