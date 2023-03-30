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

//Das sind convenience definitionen, dass wir nicht so viel tippen müssen
using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace clang;

//Die commandozeilen optionen die wir parsen wollen, gehören zu unserem program
static cl::OptionCategory API_Analysis("API_Analysis");
static cl::extrahelp MoreHelp("\nOptional extra help message");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

struct FunctionInstance {
    std::string name;
    std::string returnType;
    std::list<std::string> params;
    std::list<std::string> stmts;
    // TO-DO: include the current namespace of the function
};
std::map<std::string, FunctionInstance> oldProgram;
std::map<std::string, FunctionInstance> newProgram;
bool oldProcessed = false;

//Der visitor ist der teil, der tatsächlich die knoten des AST besucht.
class APIAnalysisVisitor : public clang::RecursiveASTVisitor<APIAnalysisVisitor> {
public:
    explicit APIAnalysisVisitor(clang::ASTContext *Context) : Context(Context){}

    //This visits function declarations with the visitor pattern
    bool VisitFunctionDecl(clang::FunctionDecl *functionDecl){
        // skip this function, if its declared in the header files
        if(!Context->getSourceManager().isInMainFile(functionDecl->getLocation())){
            return true;
        }

        FunctionInstance functionInstance;
        functionInstance.name = functionDecl->getQualifiedNameAsString();
        functionInstance.returnType = functionDecl->getDeclaredReturnType().getAsString();
        int numParam = functionDecl->getNumParams();
        for(int i=0;i<numParam;i++){
            functionInstance.params.push_back(functionDecl->getParamDecl(i)->getType().getAsString());
        }
        // Possibility: By File Name OR by class?
        FullSourceLoc FullLocation = Context->getFullLoc(functionDecl->getBeginLoc());
        FullLocation.getManager().getFilename(functionDecl->getBeginLoc());
        // Possibility: By Namespace -> no idea how I can implement this
        //
        //
        for(auto x : getStmtChildren(functionDecl->getBody())){
            functionInstance.stmts.push_back(x->getStmtClassName());
        }

        if (!oldProcessed) {
            oldProgram.insert(std::make_pair(functionInstance.name, functionInstance));
        } else {
            newProgram.insert(std::make_pair(functionInstance.name, functionInstance));
        }
        return true;
    };

private:
    clang::ASTContext *Context;
};

//Der consumer bekommt den ganzen AST und verarbeitet den
class APIAnalysisConsumer : public clang::ASTConsumer {
public:
    explicit APIAnalysisConsumer(clang::ASTContext *Context) : apiAnalysisVisitor(Context){}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        apiAnalysisVisitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    APIAnalysisVisitor apiAnalysisVisitor;
};

//Die analyse Action ist die Klasse die managed was gemacht wird, 
//In unserem fall generiert sie einen Consumer und übergibt dem die nötigen parameter
class APIAnalysisAction : public clang::ASTFrontendAction {
public:
    APIAnalysisAction(){};

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::make_unique<APIAnalysisConsumer>(&Compiler.getASTContext());
    }

private:
};

void compareFunctionHeader(FunctionInstance func);
std::string findBody(FunctionInstance oldBody);
std::string compareParams(FunctionInstance func, FunctionInstance newFunc);
std::string compareReturnType(FunctionInstance func, FunctionInstance newFunc);

void compareVersions(){
    for(auto const& x : oldProgram){
        FunctionInstance func = x.second;
        if(newProgram.count(func.name) <= 0){
            std::string bodyStatus = findBody(func);
            if(bodyStatus == ""){
                outs()<<"The function \"" + func.name + "\" was deleted\n";
            } else {
                outs()<<"The function \"" + func.name + "\" was renamed to \"" + bodyStatus + "\"\n";
            }
        }
        else {
            compareFunctionHeader(func);
        }
    }
}

std::string findBody(FunctionInstance oldFunc){
    for(auto const& x : newProgram){
        FunctionInstance func = x.second;
        if(func.stmts.size() != oldFunc.stmts.size()) continue;
        std::string matchingFunction = func.name;
        for(std::list<std::string>::iterator newIt = func.stmts.begin(), oldIt = oldFunc.stmts.begin();
            newIt!=func.stmts.end() && oldIt!=oldFunc.stmts.end();
            ++newIt, ++oldIt){
            if((*newIt) != (*oldIt)){
                matchingFunction = "";
            }
        }
        if(matchingFunction != "" && compareReturnType(oldFunc, func)=="" && compareParams(oldFunc, func) == ""){
            return matchingFunction;
        }
    }
    return "";
}

void compareFunctionHeader(FunctionInstance func){
    FunctionInstance newFunc = newProgram.at(func.name);

    // compare the Params
    outs()<<compareParams(func, newFunc);

    // compare the return type
    outs()<<compareReturnType(func, newFunc);

    // TO-DO Compare the scope of the functions
}

std::string compareParams(FunctionInstance func, FunctionInstance newFunc){
    int numberOldParams = func.params.size();
    int numberNewParams = newFunc.params.size();
    std::string output = "";
    if(numberOldParams == numberNewParams){
        for(std::list<std::string>::iterator newIt = newFunc.params.begin(), oldIt = func.params.begin();
            newIt!=newFunc.params.end() && oldIt!=func.params.end();
            ++newIt, ++oldIt){
            // TO-DO: Special Message if the order was simply changed
            if((*oldIt) != (*newIt)){
                //TO-DO: maybe insert the parameter names additionally to the types?
                output.append("The parameter type \"" + (*oldIt) + "\" of the function \"" + newFunc.name + "\" has changed to \"" + (*newIt) + "\"\n");
            }
        }
    } else {
        //TO-DO: insert a message, that shows which new Parameters where added / what the current stand is
        output.append("The function \"" + func.name + "\" has a new number of parameters. Instead of " + std::to_string(numberOldParams) + " it now has " + std::to_string(numberNewParams) + "\n");
    }
    return output;
}

std::string compareReturnType(FunctionInstance func, FunctionInstance newFunc){
    return newFunc.returnType != func.returnType ? "The function \"" + func.name + "\" has a new return Type. Instead of \"" + func.returnType + "\" it is now: \"" + newFunc.returnType + "\"\n" : "";
}

// TO-DO:
int main(int argc, const char **argv) {
    //Wir wollen commandozeilen parameter verarbeiten können, also nehmen wir einen option parser
    //Da beim parsen etwas schief gehen kann, machen wir den extra schritt mit expectation
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, API_Analysis);
    if (!ExpectedParser) {
        // Fail gracefully for unsupported options.
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }

    //Das ist der eigentliche option parser
    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    //Wir erstellen ein clang tool objekt
    ClangTool oldTool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList().at(0));
    //Wir führen die frontend action mit unserem tool aus
    oldTool.run(newFrontendActionFactory<APIAnalysisAction>().get());

    oldProcessed = true;
    ClangTool newTool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList().at(1));
    //Wir führen die frontend action mit unserem tool aus
    newTool.run(newFrontendActionFactory<APIAnalysisAction>().get());

    compareVersions();
    outs()<<"\n";
    return 0;
}
