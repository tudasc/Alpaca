//Wir wollen in der lage sein commandozeilen parameter zu verarbeiten
#include <clang/Tooling/CommonOptionsParser.h>
//Wir sind eine frontend action
#include <clang/Frontend/FrontendActions.h>
//Wir machen Tooling
#include "clang/Tooling/Tooling.h"
//Wir benutzen die Visitor strategy
#include <clang/AST/RecursiveASTVisitor.h>
//Wir müssen mit dem compiler interagieren können.
#include <clang/Frontend/CompilerInstance.h>

#include <llvm/Support/CommandLine.h>

#include "header/HelperFunctions.h"
#include "header/Analyser.h"

//Das sind convenience definitionen, dass wir nicht so viel tippen müssen
using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace clang;
using namespace helper;
using namespace analyse;

//Die commandozeilen optionen die wir parsen wollen, gehören zu unserem program
static cl::OptionCategory API_Analysis("API_Analysis");
//static cl::extrahelp MoreHelp("\nOptional extra help message");
//static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::opt<std::string> NewDirectory("newDir", cl::desc("Specify new Directory"), cl::value_desc("Directory"), cl::Positional, cl::init("-"));

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

//Der visitor ist der teil, der tatsächlich die knoten des AST besucht.
class APIAnalysisVisitor : public clang::RecursiveASTVisitor<APIAnalysisVisitor> {
    std::map<std::string, FunctionInstance>* program;
public:
    explicit APIAnalysisVisitor(clang::ASTContext *Context, std::map<std::string, FunctionInstance>* program) : Context(Context){
        this->program = program;
    }

    //This visits function declarations with the visitor pattern
    bool VisitFunctionDecl(clang::FunctionDecl *functionDecl){
        // skip this function, if it's declared in the header files
        if(!Context->getSourceManager().isInMainFile(functionDecl->getLocation())){
            return true;
        }

        // gets a vector of all the classes / namespaces a function is part of e.g. simple::example::function() -> [simple, example]
        std::string fullName = functionDecl->getQualifiedNameAsString();
        std::string delimiter = "::";
        std::vector<std::string> namespaces;

        size_t pos = 0;
        std::string singleNamespace;
        while ((pos = fullName.find(delimiter)) != std::string::npos) {
            singleNamespace = fullName.substr(0, pos);
            namespaces.push_back(singleNamespace);
            fullName.erase(0, pos + delimiter.length());
        }

        FunctionInstance functionInstance;
        functionInstance.name = functionDecl->getNameAsString();
        functionInstance.returnType = functionDecl->getDeclaredReturnType().getAsString();
        int numParam = functionDecl->getNumParams();
        for(int i=0;i<numParam;i++){
            functionInstance.params.push_back(functionDecl->getParamDecl(i)->getType().getAsString());
        }

        // gets the File Name
        FullSourceLoc FullLocation = Context->getFullLoc(functionDecl->getBeginLoc());
        FullLocation.getManager().getFilename(functionDecl->getBeginLoc());

        // saves all the Stmt class names of the function used for a body comparison
        for(auto x : getStmtChildren(functionDecl->getBody())){
            functionInstance.stmts.push_back(x->getStmtClassName());
        }

        program->insert(std::make_pair(functionInstance.name, functionInstance));
        return true;
    };

private:
    clang::ASTContext *Context;
};

//Der consumer bekommt den ganzen AST und verarbeitet den
class APIAnalysisConsumer : public clang::ASTConsumer {
public:
    explicit APIAnalysisConsumer(clang::ASTContext *Context, std::map<std::string, FunctionInstance>* program) : apiAnalysisVisitor(Context, program){}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        apiAnalysisVisitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    APIAnalysisVisitor apiAnalysisVisitor;
};

//Die analyse Action ist die Klasse die managed was gemacht wird, 
//In unserem fall generiert sie einen Consumer und übergibt dem die nötigen parameter
class APIAnalysisAction : public clang::ASTFrontendAction {
    std::map<std::string, FunctionInstance>* p;
public:
    explicit APIAnalysisAction(std::map<std::string, FunctionInstance>* program){
        p=program;
    };

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::make_unique<APIAnalysisConsumer>(&Compiler.getASTContext(), p);
    }

private:
};

// TO-DO: Herausfinden ob / wie sich der CommonOptionsParser umgehen lässt
int main(int argc, const char **argv) {

    //Wir wollen commandozeilen parameter verarbeiten können, also nehmen wir einen option parser
    //Da beim parsen etwas schief gehen kann, machen wir den extra schritt mit expectation

    auto ExpectedParser = CommonOptionsParser::create(argc, argv, API_Analysis);
    if (!ExpectedParser) {
        // Fail gracefully for unsupported options.
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }

    // TO-DO: not a viable solution, temporary only
    std::vector<std::string> oldFiles;
    listFiles(argv[1], &oldFiles);

    outs()<<"The old directory contains " + itostr(oldFiles.size()) + " files\n";

    std::vector<std::string> newFiles;
    listFiles(NewDirectory, &newFiles);

    outs()<<"The new directory contains " + itostr(newFiles.size()) + " files\n";

    std::map<std::string, FunctionInstance> oldProgram;
    std::map<std::string, FunctionInstance> newProgram;

    //Das ist der eigentliche option parser
    CommonOptionsParser& OptionsParser = ExpectedParser.get();

    //Wir erstellen ein clang tool objekt
    ClangTool oldTool(OptionsParser.getCompilations(),
                 oldFiles);
    //Wir führen die frontend action mit unserem tool aus
    oldTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&oldProgram).get());

    ClangTool newTool(OptionsParser.getCompilations(),
                   newFiles);
    //Wir führen die frontend action mit unserem tool aus
    newTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&newProgram).get());

    Analyser analyser = Analyser(oldProgram, newProgram);

    analyser.compareVersions();
    outs()<<"\n";
    return 0;
}