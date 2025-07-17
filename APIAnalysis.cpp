#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/FrontendActions.h>
#include "clang/Tooling/Tooling.h"
#include <clang/AST/RecursiveASTVisitor.h>
#include "clang/Tooling/ArgumentsAdjusters.h"
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/CommandLine.h>

#include "include/cxxopts.hpp"
#include "omp.h"

#include "header/HelperFunctions.h"
#include "header/FunctionAnalyser.h"
#include "FunctionAnalyser.cpp"
#include "header/VariableAnalyser.h"
#include "VariableAnalyser.cpp"
#include "ObjectAnalyser.cpp"
#include "header/ObjectAnalyser.h"
#include "ConsoleOutputHandler.cpp"
#include "JSONOutputHandler.cpp"
#include "filesystem"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace clang;
using namespace helper;
using namespace functionanalysis;

int clockingThing=0;

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

int findFunctionInstance(std::vector<FunctionInstance>* program, const string& filePos){
    for(int i = 0; i < program->size(); i++){
        if(program->at(i).filePosition == filePos){
            return i;
        }
    }
    return -1;
}

string getFunctionDeclFilename(FunctionDecl* functionDecl, clang::ASTContext *Context, const string& dir){
    FullSourceLoc FullLocation = Context->getFullLoc(functionDecl->getBeginLoc());
    auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(functionDecl->getBeginLoc()).str()), dir);
    return filename;
}

vector<pair<string, pair<string, string>>> getFunctionParams(FunctionDecl* functionDecl, clang::ASTContext *Context){
    vector<pair<string, pair<string, string>>> params;
    unsigned int numParam = functionDecl->getNumParams();
    for(int i=0;i<numParam;i++){
        auto paramDecl = functionDecl->getParamDecl(i);
        std::string defaultParam;
        if(paramDecl->hasDefaultArg()){
            auto start = paramDecl->getDefaultArg()->getBeginLoc(), end = paramDecl->getDefaultArg()->getEndLoc();
            LangOptions lang;
            SourceManager *sm = &(Context->getSourceManager());
            auto endToken = Lexer::getLocForEndOfToken(end, 0, *sm, lang);
            defaultParam = std::string(sm->getCharacterData(start),
                                       sm->getCharacterData(endToken) - sm->getCharacterData(start));

        }
        params.push_back(std::make_pair(paramDecl->getType().getAsString(), std::make_pair(paramDecl->getNameAsString(), defaultParam)));
    }
    return params;
}

string getLocation(FunctionDecl* functionDecl, const string& dir){
    SourceManager &sourceManager = functionDecl->getASTContext().getSourceManager();
    SourceRange sourceRange = functionDecl->getSourceRange();
    auto relative = sourceRange.getBegin().printToString(sourceManager);
    auto startOfLoc = relative.substr(0, relative.find_last_of(':')).find_last_of(':');
    auto loc = relative.substr(startOfLoc, relative.size());
    relative = relative.substr(0, startOfLoc);
    return filesystem::relative(filesystem::path(relative), dir).string() + loc;
}

class APIAnalysisVisitor : public clang::RecursiveASTVisitor<APIAnalysisVisitor> {
    std::vector<FunctionInstance>* program;
    std::string dir;
    std::vector<variableanalysis::VariableInstance>* variables;
    std::vector<objectanalysis::ObjectInstance>* objects;
    std::vector<std::string> files;
    std::map<std::string, FunctionInstance>* mapOfDeclarations;
public:
    explicit APIAnalysisVisitor(clang::ASTContext *Context, std::vector<FunctionInstance>* program, std::string directory, std::vector<variableanalysis::VariableInstance>* variables, std::vector<objectanalysis::ObjectInstance>* objects, std::vector<std::string> files, std::map<std::string, FunctionInstance>* mapOfDeclarations) : Context(Context){
        this->program = program;
        this->dir = directory;
        this->variables = variables;
        this->objects = objects;
        this->files = files;
        this->mapOfDeclarations = mapOfDeclarations;
    }

    FunctionInstance createFunctionInstance(FunctionDecl* functionDecl){

        clockingThing++;
        if(clockingThing % 400 == 0){
            outs() << clockingThing << "\n";
        }

        FunctionInstance functionInstance;

        functionInstance.name = functionDecl->getNameAsString();
        functionInstance.returnType = functionDecl->getDeclaredReturnType().getAsString();
        // for const handling the function is cast to a CXX method (because C does not have const?)
        if (auto *methodDecl = dyn_cast<CXXMethodDecl>(functionDecl)) {
            // Access CXXMethodDecl properties
            if (methodDecl->isConst()) {
                functionInstance.isConst = true;
            }else{
                functionInstance.isConst = false;
            }
        }else{
            functionInstance.isConst = false;
        }

        if (functionDecl->isFunctionTemplateSpecialization()) {
            functionInstance.isTemplateSpec = true;
            functionInstance.isTemplateDecl = false;
            for (const auto &item: functionDecl->getTemplateSpecializationArgs()->asArray()) {
                functionInstance.templateParams.emplace_back(item.getAsType().getAsString(), make_pair("", ""));
            }
        } else if (functionDecl->getDescribedFunctionTemplate()) {
            functionInstance.isTemplateSpec = false;
            functionInstance.isTemplateDecl = true;

            for (const auto &item: *functionDecl->getDescribedFunctionTemplate()->getTemplateParameters()) {
                if(const auto parm = dyn_cast<TemplateTypeParmDecl>(item)){
                    if(parm->wasDeclaredWithTypename()){
                        string defaultValue;
                        if(parm->hasDefaultArgument()){
                            defaultValue = parm->getDefaultArgument().getAsString();
                        }
                        functionInstance.templateParams.emplace_back("typename", make_pair(parm->getNameAsString(), defaultValue));
                    }else{
                        string defaultValue;
                        if(parm->hasDefaultArgument()){
                            defaultValue = parm->getDefaultArgument().getAsString();
                        }
                        functionInstance.templateParams.emplace_back("class", make_pair(parm->getNameAsString(), defaultValue));
                    }
                }else if(const auto parm = dyn_cast<NonTypeTemplateParmDecl>(item)){
                    string defaultValue;
                    if(parm->hasDefaultArgument()){
                        auto start = parm->getDefaultArgument()->getBeginLoc(), end = parm->getDefaultArgument()->getEndLoc();
                        LangOptions lang;
                        SourceManager *sm = &(Context->getSourceManager());
                        auto endToken = Lexer::getLocForEndOfToken(end, 0, *sm, lang);
                        defaultValue = std::string(sm->getCharacterData(start),
                                                   sm->getCharacterData(endToken) - sm->getCharacterData(start));
                    }
                    functionInstance.templateParams.emplace_back(parm->getType().getAsString(), make_pair(parm->getNameAsString(), defaultValue));
                }
            }
        }else {
            functionInstance.isTemplateSpec = false;
            functionInstance.isTemplateDecl = false;
        }

        // check if extern
        if(functionDecl->getStorageClass() == clang::StorageClass::SC_Extern){
            functionInstance.storageClass = "extern";
        }
            // check if static
        else if(functionDecl->getStorageClass() == clang::StorageClass::SC_Static){
            functionInstance.storageClass = "static";
        }
        else if(functionDecl->getStorageClass() == clang::StorageClass::SC_PrivateExtern){
            functionInstance.storageClass = "private extern";
        }

        if(functionDecl->isVirtualAsWritten() && functionDecl->isPure()){
            functionInstance.memberFunctionSpecifier = "pure virtual";
        }else if(functionDecl->isVirtualAsWritten()){
            functionInstance.memberFunctionSpecifier = "virtual";
        }else if(functionDecl->isPure()){
            functionInstance.memberFunctionSpecifier = "pure";
        }

        functionInstance.params = getFunctionParams(functionDecl, Context);

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
        functionInstance.filename = getFunctionDeclFilename(functionDecl, Context, dir);

        // save the qualified name of the function
        functionInstance.qualifiedName = functionDecl->getQualifiedNameAsString();

        if(functionDecl->isThisDeclarationADefinition()){
            functionInstance.isDeclaration = false;
            // saves the function body as a string
            if(functionDecl->hasBody()) {
                auto start = functionDecl->getBody()->getBeginLoc(), end = functionDecl->getBody()->getEndLoc();
                LangOptions lang;
                SourceManager *sm = &(Context->getSourceManager());
                auto endToken = Lexer::getLocForEndOfToken(end, 0, *sm, lang);
                //if(sm->getCharacterData(endToken) - sm->getCharacterData(start)) {
                    //functionInstance.body = std::string(sm->getCharacterData(start),sm->getCharacterData(endToken) - sm->getCharacterData(start));
                //}
                SourceRange sourceRange = functionDecl->getBody()->getSourceRange();
                functionInstance.body = Lexer::getSourceText(CharSourceRange::getTokenRange(sourceRange), functionDecl->getASTContext().getSourceManager(), LangOptions(), nullptr).str();

            }
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

        SourceManager &sourceManager = functionDecl->getASTContext().getSourceManager();
        SourceRange sourceRange = functionDecl->getSourceRange();
        auto entireHeader = Lexer::getSourceText(CharSourceRange::getTokenRange(sourceRange), sourceManager, LangOptions(), nullptr).str();
        entireHeader = entireHeader.substr(0, entireHeader.find('{'));
        entireHeader.erase(std::remove(entireHeader.begin(), entireHeader.end(), '\n'), entireHeader.end());

        functionInstance.filePosition = getLocation(functionDecl, dir);

        functionInstance.fullHeader = entireHeader;

        functionInstance.declarations.push_back(functionInstance);

        return functionInstance;
    }

    bool VisitRecordDecl(clang::RecordDecl *recordDecl){
        if(!Context->getSourceManager().isInMainFile(recordDecl->getLocation())){
            return true;
        }
        objectanalysis::ObjectInstance objectInstance;

        objectInstance.name = recordDecl->getNameAsString();
        objectInstance.qualifiedName = recordDecl->getQualifiedNameAsString();

        std::string fullName = recordDecl->getQualifiedNameAsString();
        std::string delimiter = "::";

        int pos = 0;
        std::string singleNamespace;
        while ((pos = fullName.find(delimiter)) != std::string::npos) {
            singleNamespace = fullName.substr(0, pos);
            objectInstance.location.push_back(singleNamespace);
            fullName.erase(0, pos + delimiter.length());
        }

        // gets the File Name
        FullSourceLoc FullLocation = Context->getFullLoc(recordDecl->getBeginLoc());
        auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(recordDecl->getBeginLoc()).str()), dir);
        objectInstance.filename = filename;

        switch (recordDecl->getTagKind()) {
            case TTK_Struct:
                objectInstance.objectType = objectanalysis::ObjectType::STRUCT;
                break;
            case TTK_Class:
                objectInstance.objectType = objectanalysis::ObjectType::CLASS;
                break;
            case TTK_Union:
                objectInstance.objectType = objectanalysis::ObjectType::ENUM_UNION;
                break;
            case TTK_Enum:
                objectInstance.objectType = objectanalysis::ObjectType::ENUM;
                break;
            default:
                objectInstance.objectType = objectanalysis::ObjectType::UNKNOWN;
                break;
        }

        if(recordDecl->getTagKind() == TTK_Class) {
            auto *cxxRecordDecl = dyn_cast<CXXRecordDecl>(recordDecl);
            if(cxxRecordDecl && cxxRecordDecl->hasDefinition()) {
                objectInstance.isAbstract = cxxRecordDecl->isAbstract();
            }else{
                objectInstance.isAbstract = false;
            }
            objectInstance.isFinal = cxxRecordDecl->isEffectivelyFinal();
        }else{
            objectInstance.isAbstract = false;
            objectInstance.isFinal = false;
        }

        this->objects->push_back(objectInstance);

        return true;
    }

    bool VisitFunctionDecl(clang::FunctionDecl *functionDecl){
        if(!getAccessSpelling(functionDecl->getAccess()).empty() && getAccessSpelling(functionDecl->getAccess()) != "public"){
            return true;
        }

        auto filename = getFunctionDeclFilename(functionDecl, Context, dir);
        if(std::find(files.begin(), files.end(), filename) == files.end()){
            return true;
        }

        if(!functionDecl->isThisDeclarationADefinition()){
            if(functionDecl->isDefined() || functionDecl->getDefinition()){
                FunctionInstance functionInstance;
                if(mapOfDeclarations->find(getLocation(functionDecl, dir)) != mapOfDeclarations->end()){
                    functionInstance = mapOfDeclarations->at(getLocation(functionDecl, dir));
                }else{
                    functionInstance = createFunctionInstance(functionDecl);
                    mapOfDeclarations->insert(make_pair(getLocation(functionDecl, dir), functionInstance));
                }

                auto position = findFunctionInstance(program, getLocation(functionDecl->getDefinition(), dir));
                if(position != -1){
                    program->at(position).declarations.push_back(functionInstance);
                }else{
                    auto definition = createFunctionInstance(functionDecl->getDefinition());
                    definition.declarations.push_back(functionInstance);
                    program->push_back(definition);
                }
            }else{
                if(mapOfDeclarations->find(getLocation(functionDecl, dir)) != mapOfDeclarations->end()){
                    return true;
                }
                auto functionInstance = createFunctionInstance(functionDecl);
                mapOfDeclarations->insert(make_pair(getLocation(functionDecl, dir), functionInstance));
            }
        }else{
            if(findFunctionInstance(program, getLocation(functionDecl->getDefinition(), dir)) == -1){
                auto functionInstance = createFunctionInstance(functionDecl);
                program->push_back(functionInstance);
            }
            /*
            if(!functionDecl->decls_empty()){
                for(auto temp = functionDecl->decls_begin(); temp != functionDecl->decls_end(); temp++){
                    if(auto decl = temp->getAsFunction()) {
                        functionInstance.declarations.push_back(createFunctionInstance(decl));
                    }
                }
            }
            */
        }
        return true;
    };

    bool VisitVarDecl(clang::VarDecl* varDecl){
        // global variables of the files (i.e. not class / struct members)
        if(!Context->getSourceManager().isInMainFile(varDecl->getLocation()) || !varDecl->hasGlobalStorage()){
            return true;
        }

        variableanalysis::VariableInstance variableInstance;

        if(varDecl->isThisDeclarationADefinition()){
            variableInstance.isDefinition = true;
            if(varDecl->getEvaluatedValue()){
                variableInstance.defaultValue = varDecl->getEvaluatedValue()->getAsString(*Context, varDecl->getType());
            }
        }else{
            variableInstance.isDefinition = false;
        }

        variableInstance.isClassMember = false;

        variableInstance.name = varDecl->getNameAsString();
        variableInstance.qualifiedName = varDecl->getQualifiedNameAsString();

        auto fullType = varDecl->getType().getAsString();
        // TODO: well...
        if(fullType.find(" at ") != std::string::npos && fullType.find("(unnamed ") != std::string::npos){
            fullType = fullType.substr(0, fullType.find(" at ")) + ")";
        }
        variableInstance.type = fullType;

        // gets the File Name
        FullSourceLoc FullLocation = Context->getFullLoc(varDecl->getBeginLoc());
        auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(varDecl->getBeginLoc()).str()), dir);
        variableInstance.filename = filename;

        // gets a vector of all the classes / namespaces a variable is part of e.g. simple::example::function() -> [simple, example]
        std::string fullName = varDecl->getQualifiedNameAsString();
        std::string delimiter = "::";

        int pos = 0;
        std::string singleNamespace;
        while ((pos = fullName.find(delimiter)) != std::string::npos) {
            singleNamespace = fullName.substr(0, pos);
            variableInstance.location.push_back(singleNamespace);
            fullName.erase(0, pos + delimiter.length());
        }

        // check if extern
        if(varDecl->getStorageClass() == clang::StorageClass::SC_Extern){
            variableInstance.storageClass = "extern";
        }
            // check if static
        else if(varDecl->getStorageClass() == clang::StorageClass::SC_Static){
            variableInstance.storageClass = "static";
        }
        else if(varDecl->getStorageClass() == clang::StorageClass::SC_PrivateExtern){
            variableInstance.storageClass = "private extern";
        }
        else if(varDecl->getStorageClass() == clang::StorageClass::SC_Register){
            variableInstance.storageClass = "register";
        }

        if(varDecl->isInline()){
            variableInstance.isInline = true;
        }else{
            variableInstance.isInline = false;
        }

        QualType type = varDecl->getType();
        if (type.isConstQualified()) {
            variableInstance.isConst = true;
        }else{
            variableInstance.isConst = false;
        }

        if(type.hasQualifiers()) {
            variableInstance.qualifiers = type.getQualifiers().getAsString();
        }

        // TODO: not a nice implementation, but it works for now
        if(variableInstance.qualifiers.find("explicit") != std::string::npos){
            variableInstance.isExplicit = true;
        }else{
            variableInstance.isExplicit = false;
        }

        if(variableInstance.qualifiers.find("volatile") != std::string::npos){
            variableInstance.isVolatile = true;
        }else{
            variableInstance.isVolatile = false;
        }

        if(variableInstance.qualifiers.find("mutable") != std::string::npos){
            variableInstance.isMutable = true;
        }else{
            variableInstance.isMutable = false;
        }

        variables->push_back(variableInstance);

        return true;
    }

    bool VisitFieldDecl(clang::FieldDecl* decl){
        // class / struct members
        if(!Context->getSourceManager().isInMainFile(decl->getLocation())){
            return true;
        }

        variableanalysis::VariableInstance variableInstance;
        variableInstance.isClassMember = false;
        variableInstance.isInline = false;
        variableInstance.isDefinition = false;

        variableInstance.name = decl->getNameAsString();
        variableInstance.qualifiedName = decl->getQualifiedNameAsString();
        auto fullType = decl->getType().getAsString();
        // TODO: well...
        if(fullType.find(" at ") != std::string::npos && fullType.find("(unnamed ") != std::string::npos){
            fullType = fullType.substr(0, fullType.find(" at ")) + ")";
        }
        variableInstance.type = fullType;
        if(decl->hasInClassInitializer()){
            auto expr = decl->getInClassInitializer();
            APValue apValue;
            if (expr && expr->isCXX11ConstantExpr(*Context, &apValue)) {
                variableInstance.defaultValue = apValue.getAsString(*Context, decl->getType());
            } else {
                variableInstance.defaultValue = "";
            }
            variableInstance.defaultValue = "";
        }
        // gets the File Name
        FullSourceLoc FullLocation = Context->getFullLoc(decl->getBeginLoc());
        auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(decl->getBeginLoc()).str()), dir);
        variableInstance.filename = filename;

        // gets a vector of all the classes / namespaces a variable is part of e.g. simple::example::function() -> [simple, example]
        std::string fullName = decl->getQualifiedNameAsString();
        std::string delimiter = "::";

        int pos = 0;
        std::string singleNamespace;
        while ((pos = fullName.find(delimiter)) != std::string::npos) {
            singleNamespace = fullName.substr(0, pos);
            variableInstance.location.push_back(singleNamespace);
            fullName.erase(0, pos + delimiter.length());
        }

        QualType type = decl->getType();
        if (type.isConstQualified()) {
            variableInstance.isConst = true;
        }else{
            variableInstance.isConst = false;
        }

        if(type.hasQualifiers()) {
            variableInstance.qualifiers = type.getQualifiers().getAsString();
        }

        // TODO: not a nice implementation, but it works for now
        if(variableInstance.qualifiers.find("explicit") != std::string::npos){
            variableInstance.isExplicit = true;
        }else{
            variableInstance.isExplicit = false;
        }

        if(variableInstance.qualifiers.find("volatile") != std::string::npos){
            variableInstance.isVolatile = true;
        }else{
            variableInstance.isVolatile = false;
        }

        if(variableInstance.qualifiers.find("mutable") != std::string::npos){
            variableInstance.isMutable = true;
        }else{
            variableInstance.isMutable = false;
        }

        if(decl->getAccess() == clang::AccessSpecifier::AS_none){
            variableInstance.accessSpecifier = "none";
        }else if(decl->getAccess() == clang::AccessSpecifier::AS_private){
            variableInstance.accessSpecifier = "private";
        }else if(decl->getAccess() == clang::AccessSpecifier::AS_protected) {
            variableInstance.accessSpecifier = "protected";
        }else if(decl->getAccess() == clang::AccessSpecifier::AS_public){
            variableInstance.accessSpecifier = "public";
        }

        variables->push_back(variableInstance);

        return true;
    }

private:
    clang::ASTContext *Context;
};


class APIAnalysisConsumer : public clang::ASTConsumer {
public:
    explicit APIAnalysisConsumer(clang::ASTContext *Context, std::vector<FunctionInstance>* program, std::string dir, std::vector<variableanalysis::VariableInstance>* var, std::vector<objectanalysis::ObjectInstance>* objects, std::vector<std::string>& files, std::map<std::string, FunctionInstance>* mapOfDeclarations) : apiAnalysisVisitor(Context, program, dir, var, objects, files, mapOfDeclarations){}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        //outs()<<"File is: " + filesystem::current_path().string()<<"\n";
        apiAnalysisVisitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    APIAnalysisVisitor apiAnalysisVisitor;
};


class APIAnalysisAction : public clang::ASTFrontendAction {
    std::vector<FunctionInstance>* p;
    std::string directory;
    std::vector<variableanalysis::VariableInstance>* var;
    std::vector<objectanalysis::ObjectInstance>* obj;
    std::vector<std::string> files;
    std::map<std::string, FunctionInstance>* mapOfDeclarations;
public:
    explicit APIAnalysisAction(std::vector<FunctionInstance>* program, std::string dir, std::vector<variableanalysis::VariableInstance>* var, std::vector<objectanalysis::ObjectInstance>* objects, std::vector<std::string> files, std::map<std::string, FunctionInstance>* mapOfDeclarations){
        this->p=program;
        this->directory = dir;
        this->var = var;
        this->obj = objects;
        this->files = files;
        this->mapOfDeclarations = mapOfDeclarations;
    };

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        Compiler.getDiagnostics().setClient(new clang::IgnoringDiagConsumer(), true);
        return std::make_unique<APIAnalysisConsumer>(&Compiler.getASTContext(), p, directory, var, obj, files, mapOfDeclarations);
    }

private:
};

std::vector<FunctionInstance> assignDeclarations(std::vector<FunctionInstance>& functions){
    std::vector<FunctionInstance> output;
    std::vector<FunctionInstance> usedDeclarations;
    for (auto &item: functions){
        if(item.isDeclaration){
            continue;
        }
        // find all instances of this particular qualified name
        for (auto &otherItem : functions)
        {
            if(otherItem.qualifiedName != item.qualifiedName){
                continue;
            }
            // if the current is a matching declaration, it is added to the FunctionItem item
            if(item.isCorrectDeclaration(otherItem)){
                item.declarations.push_back(otherItem);
                usedDeclarations.push_back(otherItem);
            }
        }
        item.declarations.push_back(item);
    }

    // delete all the declarations from the list
    for (int i=0;i<functions.size();i++)
    {
        // adds definitions to the output
        if (!(functions.at(i).isDeclaration && std::find_if(usedDeclarations.begin(), usedDeclarations.end(),[functions, i](const FunctionInstance& func){return functions.at(i).qualifiedName == func.qualifiedName;})!=usedDeclarations.end())) {
            // any declaration that is still included has no definition and therefore should also reference itself in the declaration vector (to minimize border case handling the analyser)
            if(functions.at(i).isDeclaration){
                functions.at(i).declarations.push_back(functions.at(i));
            }
            output.push_back(functions.at(i));
        }
    }

    return output;
}

std::vector<variableanalysis::VariableInstance> assignDeclarations(std::vector<variableanalysis::VariableInstance>& variables){
    clockingThing = 0;
    std::vector<variableanalysis::VariableInstance> output;
    std::vector<variableanalysis::VariableInstance> usedDefinitions;
    for (auto &item: variables){
        if(item.isDefinition){
            continue;
        }
        // find all instances of this particular qualified name
        for (auto &otherItem : variables)
        {
            if(otherItem.qualifiedName != item.qualifiedName || item.filename == otherItem.filename){
                continue;
            }
            item.definitions.push_back(otherItem);
            usedDefinitions.push_back(otherItem);
        }
    }

    // delete all the declarations from the list
    for (auto & variable : variables)
    {
        if(clockingThing++%500==0){
            outs()<<clockingThing<<"\n";
        }
        // adds definitions to the output
        if (!(variable.isDefinition && std::find_if(usedDefinitions.begin(), usedDefinitions.end(),[variables, &variable](const variableanalysis::VariableInstance& var){return variable.qualifiedName == var.qualifiedName;})!=usedDefinitions.end())) {
            output.push_back(variable);
        }
    }

    return output;
}

std::vector<FunctionInstance> assignSpecializations(std::vector<FunctionInstance>& funcs){
    std::vector<FunctionInstance> output;
    std::vector<FunctionInstance> usedSpecializations;
    for (auto &item: funcs){
        if(item.isTemplateSpec){
            continue;
        }
        // find all instances of this particular qualified name
        for (auto &otherItem : funcs)
        {
            if(!otherItem.isTemplateSpec || otherItem.qualifiedName != item.qualifiedName || item.params.size() != otherItem.params.size()){
                continue;
            }
            item.templateSpecializations.push_back(otherItem);
            usedSpecializations.push_back(otherItem);
        }
    }

    // delete all the declarations from the list
    for (auto & i : funcs)
    {
        // adds definitions to the output
        if (!(i.isTemplateSpec && std::find_if(usedSpecializations.begin(), usedSpecializations.end(),[funcs, &i](const FunctionInstance& func){return i.qualifiedName == func.qualifiedName;})!=usedSpecializations.end())) {
            output.push_back(i);
        }
    }

    return output;
}

void insertUndefinedDeclarations(vector<FunctionInstance>* definitions, map<std::string, FunctionInstance>* declarations) {
    outs()<<"time check start\n";
    /*
    for (auto &def: *definitions){
        for (const auto &decl: *declarations){
            if(def.qualifiedName == "two_phase_exch_and_write" && decl.second.qualifiedName == "two_phase_exch_and_write"){
                outs() << "here\n";
                auto check = def.isCorrectDeclaration(decl.second);
            }
            if(def.qualifiedName == decl.second.qualifiedName){
                if(def.isCorrectDeclaration(decl.second)){
                    def.declarations.push_back(decl.second);
                }
            }
        }
    }
    */
    outs()<<"time check end\n";
    outs()<<"Started with " << declarations->size() << " declarations\n";
    for (auto &def : *definitions) {
        for (const auto &decl: def.declarations){
            declarations->erase(decl.filePosition);
        }
    }

    outs()<<"There are " << declarations->size() << " undefined declarations\n";
    for (auto &item: *declarations){
        definitions->push_back(item.second);
    }
}

int main(int argc, const char **argv) {
    cxxopts::Options options("APIAnalysis", "Compares two versions of a C++ API and prints out the differences");
    options.add_options()
            ("doc, deep-overload-comparison", "Enables the statistical comparison of function bodies")
            ("ipf,include-private-functions", "Include private functions in the output")
            ("json,output-JSON", "Output the functionanalysis in a custom JSON format")
            ("newDir, newDirectory", "Path to the newer version of the project - REQUIRED", cxxopts::value<std::string>())
            ("oldDir, oldDirectory", "Path to the older version of the project - REQUIRED", cxxopts::value<std::string>())
            ("oldCD, oldCompilationDatabase", "Path to the compilation database of the old directory", cxxopts::value<std::string>())
            ("newCD, newCompilationDatabase", "Path to the compilation database of the new directory", cxxopts::value<std::string>())
            ("extra-args, extra-arguments", "Add additional clang arguments for both projects, separated with ,", cxxopts::value<std::vector<std::string>>())
            ("extra-args-old, extra-arguments-old", "Add additional clang arguments for the old project, separated with ,", cxxopts::value<std::vector<std::string>>())
            ("extra-args-new, extra-arguments-new", "Add additional clang arguments for the new project, separated with ,", cxxopts::value<std::vector<std::string>>())
            ("exclude, exc", "Add directories or files [separated with a comma] that should be ignored (relative Path from the root of the directory) WARNING: only use if the project structure has not changed", cxxopts::value<std::vector<std::string>>())
            ("exclude-new, excN", "Add directories or files [separated with a comma] of the new program that should be ignored (relative Path from the given new directory)", cxxopts::value<std::vector<std::string>>())
            ("exclude-old, excO", "Add directories or files [separated with a comma] of the old program that should be ignored (relative Path from the given old directory)", cxxopts::value<std::vector<std::string>>())
            ("h,help", "Print usage")
            ;
    auto result = options.parse(argc, argv);

    if(result.count("help")){
        outs()<<options.help();
        return 0;
    }

    vector<string> oldExcludedItems;
    vector<string> newExcludedItems;
    if(result["exclude"].count()) {
        for (auto &item : result["exclude"].as<std::vector<std::string>>()) {
            oldExcludedItems.push_back(item);
            newExcludedItems.push_back(item);
        }
    }

    if(result["exclude-new"].count()) {
        for (auto &item : result["exclude-new"].as<std::vector<std::string>>()) {
            newExcludedItems.push_back(item);
        }
    }

    if(result["exclude-old"].count()) {
        for (auto &item : result["exclude-old"].as<std::vector<std::string>>()) {
            oldExcludedItems.push_back(item);
        }
    }

    std::vector<std::string> oldFiles;
    std::vector<std::string> newFiles;

    if(result.count("oldDir")){
        listFiles(result["oldDir"].as<std::string>(), &oldFiles, &oldExcludedItems);
        outs()<<"The old directory contains " + itostr(oldFiles.size()) + " files\n";

    }else{
        throw std::invalid_argument("The older version of the project has to be specified");
    }

    if(result.count("newDir")){
        listFiles(result["newDir"].as<std::string>(), &newFiles, &newExcludedItems);
        outs()<<"The new directory contains " + itostr(newFiles.size()) + " files\n";
    }else{
        throw std::invalid_argument("The older version of the project has to be specified (--oldDir has to be set)");
    }

    bool docEnabled = result["doc"].as<bool>();

    bool outputPrivateFunctions = result["ipf"].as<bool>();

    bool jsonOutput = result["json"].as<bool>();

    // lists of functions
    std::vector<FunctionInstance> oldProgram;
    std::vector<FunctionInstance> newProgram;

    // lists of variables
    std::vector<variableanalysis::VariableInstance> oldVariables;
    std::vector<variableanalysis::VariableInstance> newVariables;

    // lists of objects
    std::vector<objectanalysis::ObjectInstance> oldObjects;
    std::vector<objectanalysis::ObjectInstance> newObjects;

    // compilation Databases
    std::unique_ptr<CompilationDatabase> oldCD;
    std::unique_ptr<CompilationDatabase> newCD;

    if(result.count("oldCD")){
        std::string errorMessage = "Could not load the specified old compilation Database, trying to find one in the project files\n";
        oldCD = FixedCompilationDatabase::autoDetectFromDirectory(std::filesystem::canonical(result["oldCD"].as<std::string>()).string(), errorMessage);
        oldFiles.clear();
        oldFiles = oldCD->getAllFiles();
        outs()<<oldFiles.size();
    }

    if(result.count("newCD")){
        std::string errorMessage = "Could not load the specified new compilation Database, trying to find one in the project files\n";
        newCD = FixedCompilationDatabase::autoDetectFromDirectory(std::filesystem::canonical(result["newCD"].as<std::string>()).string(), errorMessage);
        newFiles.clear();
        newFiles = newCD->getAllFiles();
        outs()<<newFiles.size();
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

    //ClangTool oldTool(*oldCD,oldFiles);

    std::vector<std::string> relativeListOfOldFiles;
    std::vector<std::string> relativeListOfNewFiles;

    std::map<std::string, FunctionInstance> mapOfDeclarations;

    relativeListOfOldFiles.reserve(oldFiles.size());
    for (const auto &item: oldFiles){
        relativeListOfOldFiles.push_back(std::filesystem::relative(item, std::filesystem::canonical(std::filesystem::absolute(result["oldDir"].as<std::string>()))).string());
    }
    relativeListOfNewFiles.reserve(newFiles.size());
    for (const auto &item: newFiles){
        relativeListOfNewFiles.push_back(std::filesystem::relative(item, std::filesystem::canonical(std::filesystem::absolute(result["newDir"].as<std::string>()))).string());
    }

    for (const auto &item: oldFiles){
        ClangTool oldTool = ClangTool(*oldCD,
                            std::vector<std::string>{item});
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
        oldTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&oldProgram, std::filesystem::canonical(std::filesystem::absolute(result["oldDir"].as<std::string>())), &oldVariables, &oldObjects, relativeListOfOldFiles, &mapOfDeclarations).get());
    }
    /*
    for (int i=0; i<oldFiles.size(); i++){
        if(oldFiles.at(i) == "/home/paul/Downloads/openmpi-4.1.52/opal/mca/pmix/pmix3x/pmix/src/threads/thread.c"){
            outs()<<"found the file\n";
            outs()<<oldFiles.at(i+1);
        }
    }

    ClangTool oldTool = ClangTool(*oldCD,
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
    oldTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&oldProgram, std::filesystem::canonical(std::filesystem::absolute(result["oldDir"].as<std::string>())), &oldVariables, &oldObjects, relativeListOfOldFiles, &mapOfDeclarations).get());
    */
    insertUndefinedDeclarations(&oldProgram, &mapOfDeclarations);

    mapOfDeclarations.clear();

    //ClangTool newTool(*newCD,newFiles);
    for (const auto &item: newFiles){
        ClangTool newTool = ClangTool(*newCD,
                                      std::vector<std::string>{item});
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
        newTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&newProgram, std::filesystem::canonical(std::filesystem::absolute(result["newDir"].as<std::string>())), &newVariables, &newObjects, relativeListOfNewFiles, &mapOfDeclarations).get());
    }
    /*
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

    newTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&newProgram, std::filesystem::canonical(std::filesystem::absolute(result["newDir"].as<std::string>())), &newVariables, &newObjects, relativeListOfNewFiles, &mapOfDeclarations).get());
    */
    insertUndefinedDeclarations(&newProgram, &mapOfDeclarations);

    outs()<<"All functions, objects and variables were processed\n";
    outs()<<"In total "<<oldProgram.size()<<" functions, "<<oldObjects.size()<<" objects and "<<oldVariables.size()<<" variables were found in the old version of the project\n";
    outs()<<"In total "<<newProgram.size()<<" functions, "<<newObjects.size()<<" objects and "<<newVariables.size()<<" variables were found in the new version of the project\n";

    // TODO: and i fear, same here (that rhymes :D)
    oldProgram = assignSpecializations(oldProgram);
    newProgram = assignSpecializations(newProgram);

    // TODO: same problem as the function equivalent, it takes way to long..
    //oldVariables = assignDeclarations(oldVariables);
    //newVariables = assignDeclarations(newVariables);

    OutputHandler* outputHandler;
    if(jsonOutput){
        outputHandler = new JSONOutputHandler();
    }else{
        outputHandler = new ConsoleOutputHandler();
    }
    outs()<<"The objectanalysis started with "<< oldObjects.size() << "objects \n";
    // Analysing Objects
    objectanalysis::ObjectAnalyser objectAnalyser = objectanalysis::ObjectAnalyser(oldObjects, newObjects, outputHandler);
    //objectAnalyser.compareObjects();
    outs()<<"The functionanalysis started " << oldProgram.size() << " funcs \n";
    // Analysing Functions
    FunctionAnalyser analyser = FunctionAnalyser(oldProgram, newProgram, outputHandler);
    analyser.compareVersionsWithDoc(docEnabled, outputPrivateFunctions);
    outs()<<"The variableanalysis started " << oldVariables.size() << " variables \n";
    // Analysing Variables
    variableanalysis::VariableAnalyser variableAnalyser = variableanalysis::VariableAnalyser(oldVariables, newVariables, outputHandler);
    //variableAnalyser.compareVariables();

    outputHandler->printOut();

    return 0;
}
