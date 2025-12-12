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
using namespace variableanalysis;

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

int findVariableInstance(std::vector<VariableInstance>* program, const string& filePos){
    for(int i = 0; i < program->size(); i++){
        if(program->at(i).filePosition == filePos){
            return i;
        }
    }
    return -1;
}

int findObjectInstance(std::vector<ObjectInstance>* program, const string& filePos){
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
    return filename.string();
}

string getVarDeclFilename(VarDecl* varDecl, clang::ASTContext *Context, const string& dir){
    FullSourceLoc FullLocation = Context->getFullLoc(varDecl->getBeginLoc());
    auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(varDecl->getBeginLoc()).str()), dir);
    return filename.string();
}

string getVarDeclFilename(FieldDecl* varDecl, clang::ASTContext *Context, const string& dir){
    FullSourceLoc FullLocation = Context->getFullLoc(varDecl->getBeginLoc());
    auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(varDecl->getBeginLoc()).str()), dir);
    return filename.string();
}

string getRecordDeclFilename(RecordDecl* recordDecl, clang::ASTContext *Context, const string& dir){
    FullSourceLoc FullLocation = Context->getFullLoc(recordDecl->getBeginLoc());
    auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(recordDecl->getBeginLoc()).str()), dir);
    return filename.string();
}

vector<pair<string, pair<string, string>>> getFunctionParams(FunctionDecl* functionDecl, clang::ASTContext *Context){
    vector<pair<string, pair<string, string>>> params;
    unsigned int numParam = functionDecl->getNumParams();
    for(int i=0;i<numParam;i++){
        auto paramDecl = functionDecl->getParamDecl(i);
        std::string defaultParam;
        if(paramDecl->hasDefaultArg()){
            // get the default argument as a string without using the Lexer
            std::string defaultArgStr;
            llvm::raw_string_ostream defaultArgStream(defaultArgStr);
            if(paramDecl->getDefaultArg()) {
                paramDecl->getDefaultArg()->printPretty(defaultArgStream, nullptr,
                                                        PrintingPolicy(Context->getLangOpts()));
                defaultParam = defaultArgStream.str();
            }else {
                defaultParam = "";
            }


            /*
            auto test = paramDecl->getDefaultArg();
            auto name = paramDecl->getNameAsString();
            auto location = functionDecl->getSourceRange();
            auto filename = getFunctionDeclFilename(functionDecl, Context, "");
            auto start = paramDecl->getDefaultArg()->getBeginLoc();
            auto end = paramDecl->getDefaultArg()->getEndLoc();
            SourceManager *sm = &(Context->getSourceManager());
            auto endToken = Lexer::getLocForEndOfToken(end, 0, *sm, Context->getLangOpts());
            defaultParam = std::string(sm->getCharacterData(start),
                                       sm->getCharacterData(endToken) - sm->getCharacterData(start));
            */

        }
        params.emplace_back(paramDecl->getType().getAsString(), std::make_pair(paramDecl->getNameAsString(), defaultParam));
    }
    return params;
}

string getLocation(FunctionDecl* functionDecl, const string& dir, clang::ASTContext* context){
    SourceManager &sourceManager = context->getSourceManager();
    SourceRange sourceRange = functionDecl->getSourceRange();
    return getFunctionDeclFilename(functionDecl, context, dir) + ":" + to_string(sourceManager.getPresumedLineNumber(sourceRange.getBegin())) + ":" + to_string(sourceManager.getPresumedColumnNumber(sourceRange.getBegin()));
}

string getLocation(VarDecl* varDecl, const string& dir, clang::ASTContext* context){
    SourceManager &sourceManager = context->getSourceManager();
    SourceRange sourceRange = varDecl->getSourceRange();
    return getVarDeclFilename(varDecl, context, dir) + ":" + to_string(sourceManager.getPresumedLineNumber(sourceRange.getBegin())) + ":" + to_string(sourceManager.getPresumedColumnNumber(sourceRange.getBegin()));
}

string getLocation(FieldDecl* varDecl, const string& dir, clang::ASTContext* context){
    SourceManager &sourceManager = context->getSourceManager();
    SourceRange sourceRange = varDecl->getSourceRange();
    return getVarDeclFilename(varDecl, context, dir) + ":" + to_string(sourceManager.getPresumedLineNumber(sourceRange.getBegin())) + ":" + to_string(sourceManager.getPresumedColumnNumber(sourceRange.getBegin()));
}

string getLocation(RecordDecl* varDecl, const string& dir, clang::ASTContext* context){
    SourceManager &sourceManager = context->getSourceManager();
    SourceRange sourceRange = varDecl->getSourceRange();
    return getRecordDeclFilename(varDecl, context, dir) + ":" + to_string(sourceManager.getPresumedLineNumber(sourceRange.getBegin())) + ":" + to_string(sourceManager.getPresumedColumnNumber(sourceRange.getBegin()));
}

class APIAnalysisVisitor : public clang::RecursiveASTVisitor<APIAnalysisVisitor> {
    std::vector<FunctionInstance>* program;
    std::string dir;
    std::vector<variableanalysis::VariableInstance>* variables;
    std::vector<objectanalysis::ObjectInstance>* objects;
    std::vector<std::string> files;
    std::map<std::string, FunctionInstance>* mapOfDeclarations;
    std::map<std::string, VariableInstance>* variableDefinitions;
public:
    explicit APIAnalysisVisitor(clang::ASTContext *Context, std::vector<FunctionInstance>* program, std::string directory, std::vector<variableanalysis::VariableInstance>* variables, std::vector<objectanalysis::ObjectInstance>* objects, std::vector<std::string> files, std::map<std::string, FunctionInstance>* mapOfDeclarations, std::map<std::string, VariableInstance>* variableDefinitions) : Context(Context){
        this->program = program;
        this->dir = directory;
        this->variables = variables;
        this->objects = objects;
        this->files = files;
        this->mapOfDeclarations = mapOfDeclarations;
        this->variableDefinitions = variableDefinitions;
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
                //functionInstance.templateParams.emplace_back(item.getAsType().getAsString(), make_pair("", ""));
                if(item.getKind() == TemplateArgument::ArgKind::Type) {
                    functionInstance.templateParams.emplace_back(item.getAsType().getAsString(), make_pair("", ""));
                }else if(item.getKind() == TemplateArgument::ArgKind::Integral) {
                    functionInstance.templateParams.emplace_back(toString(item.getAsIntegral(),10), make_pair("", ""));
                }else if(item.getKind() == TemplateArgument::ArgKind::Declaration) {
                    functionInstance.templateParams.emplace_back(item.getAsDecl()->getType().getAsString(),
                                                                 make_pair("", ""));
                }else if(item.getKind() == TemplateArgument::ArgKind::NullPtr) {
                    functionInstance.templateParams.emplace_back("nullptr", make_pair("", ""));
                }else if(item.getKind() == TemplateArgument::ArgKind::Template) {
                    functionInstance.templateParams.emplace_back("nested template", make_pair("", ""));
                }else{
                    functionInstance.templateParams.emplace_back("unknown", make_pair("", ""));
                }
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
                        auto sourceRange = parm->getDefaultArgument()->getSourceRange();
                        //defaultValue = std::string(sm->getCharacterData(start),
                        //                           sm->getCharacterData(endToken) - sm->getCharacterData(start));
                        defaultValue = Lexer::getSourceText(CharSourceRange::getTokenRange(sourceRange), functionDecl->getASTContext().getSourceManager(), LangOptions(), nullptr).str();
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
        else if(functionDecl->getStorageClass() == clang::StorageClass::SC_Register){
            functionInstance.storageClass = "register";
        }

        if(functionDecl->isVirtualAsWritten() && functionDecl->isPure()){
            functionInstance.memberFunctionSpecifier = "pure virtual";
        }else if(functionDecl->isVirtualAsWritten()){
            functionInstance.memberFunctionSpecifier = "virtual";
        }else if(functionDecl->isPure()){
            functionInstance.memberFunctionSpecifier = "pure";
        }
        // TODO: technically doesnt belong here, but the they are mutally exclusive so works for now
        else if(functionDecl->getFriendObjectKind() == Decl::FriendObjectKind::FOK_Declared || functionDecl->getFriendObjectKind() == Decl::FriendObjectKind::FOK_Undeclared){
            functionInstance.memberFunctionSpecifier = "friend";
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

        functionInstance.filePosition = getLocation(functionDecl, dir, Context);

        functionInstance.fullHeader = entireHeader;

        functionInstance.declarations.push_back(functionInstance);

        return functionInstance;
    }

    VariableInstance createVariableInstance(VarDecl* varDecl){
        variableanalysis::VariableInstance variableInstance;

        clockingThing++;
        if(clockingThing % 400 == 0){
            outs() << clockingThing << "\n";
        }

        variableInstance.isClassMember = false;

        variableInstance.name = varDecl->getNameAsString();
        variableInstance.qualifiedName = varDecl->getQualifiedNameAsString();

        auto fullType = varDecl->getType().getAsString();
        // TODO: well... (used to fix unnamed / anonymous namespaces, classes and lambdas)
        if(fullType.find(" at ") != std::string::npos && fullType.find("(unnamed ") != std::string::npos){
            fullType = fullType.substr(0, fullType.find(" at ")) + ")";
        } else if(fullType.find(" at ") != std::string::npos && fullType.find("(lambda ") != std::string::npos){
            fullType = fullType.substr(0, fullType.find(" at ")) + ")";
        }
        std::size_t ind = fullType.find("const ");
        if(ind !=std::string::npos){
            fullType.erase(ind, 6);
        }
        variableInstance.type = fullType;

        variableInstance.filename = getVarDeclFilename(varDecl, Context, dir);

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

        variableInstance.filePosition = getLocation(varDecl, dir, Context);

        return variableInstance;
    }

    VariableInstance createVariableInstance(FieldDecl* decl){
        variableanalysis::VariableInstance variableInstance;
        variableInstance.isClassMember = true;
        variableInstance.isInline = false;

        clockingThing++;
        if(clockingThing % 400 == 0){
            outs() << clockingThing << "\n";
        }

        variableInstance.name = decl->getNameAsString();
        variableInstance.qualifiedName = decl->getQualifiedNameAsString();
        auto fullType = decl->getType().getAsString();
        // TODO: well...
        if(fullType.find(" at ") != std::string::npos && fullType.find("(unnamed ") != std::string::npos){
            fullType = fullType.substr(0, fullType.find(" at ")) + ")";
        } else if(fullType.find(" at ") != std::string::npos && fullType.find("(lambda ") != std::string::npos){
            fullType = fullType.substr(0, fullType.find(" at ")) + ")";
        }
        variableInstance.type = fullType;

        variableInstance.filename = getVarDeclFilename(decl, Context, dir);
        variableInstance.filePosition = getLocation(decl, dir, Context);

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

        return variableInstance;
    }

    ObjectInstance createObjectInstance(RecordDecl* recordDecl){
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

        objectInstance.filePosition = getLocation(recordDecl, dir, Context);

        // gets the File Name
        FullSourceLoc FullLocation = Context->getFullLoc(recordDecl->getBeginLoc());
        auto filename = std::filesystem::relative(std::filesystem::path(FullLocation.getManager().getFilename(recordDecl->getBeginLoc()).str()), dir);
        objectInstance.filename = filename;

        objectInstance.objectType = recordDecl->getTagKind();

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

        return objectInstance;
    }

    bool VisitRecordDecl(clang::RecordDecl *recordDecl){
        auto filename = getRecordDeclFilename(recordDecl, Context, dir);
        if(std::find(files.begin(), files.end(), filename) == files.end()){
            return true;
        }

        if(findObjectInstance(objects, getLocation(recordDecl, dir, Context)) != -1){
            return true;
        }

        auto objectInstance = createObjectInstance(recordDecl);
        this->objects->push_back(objectInstance);

        return true;
    }

    bool VisitFunctionDecl(clang::FunctionDecl *functionDecl){
        auto filename = getFunctionDeclFilename(functionDecl, Context, dir);
        if(std::find(files.begin(), files.end(), filename) == files.end()){
            return true;
        }

        if(!functionDecl->isThisDeclarationADefinition()){
            if(functionDecl->isDefined() || functionDecl->getDefinition()){
                auto position = findFunctionInstance(program, getLocation(functionDecl->getDefinition(), dir, Context));
                if(position != -1){
                    auto loc = getLocation(functionDecl, dir, Context);
                    for (const auto &item: program->at(position).declarations){
                        if(item.filePosition == loc){
                            return true;
                        }
                    }

                    FunctionInstance functionInstance;
                    if(mapOfDeclarations->find(getLocation(functionDecl, dir, Context)) != mapOfDeclarations->end()){
                        functionInstance = mapOfDeclarations->at(getLocation(functionDecl, dir, Context));
                    }else{
                        functionInstance = createFunctionInstance(functionDecl);
                        mapOfDeclarations->insert(make_pair(getLocation(functionDecl, dir, Context), functionInstance));
                    }

                    program->at(position).declarations.push_back(functionInstance);
                }else{
                    FunctionInstance functionInstance;
                    if(mapOfDeclarations->find(getLocation(functionDecl, dir, Context)) != mapOfDeclarations->end()){
                        functionInstance = mapOfDeclarations->at(getLocation(functionDecl, dir, Context));
                    }else{
                        functionInstance = createFunctionInstance(functionDecl);
                        mapOfDeclarations->insert(make_pair(getLocation(functionDecl, dir, Context), functionInstance));
                    }

                    // some definition filenames are empty (about 7 out of 11000 and no idea why), so the filename is set to the declaration filename
                    FunctionInstance definition;
                    if(getFunctionDeclFilename(functionDecl->getDefinition(), Context, dir) == ""){
                        auto filePosition = getLocation(functionDecl->getDefinition(), dir, Context);
                        if(findFunctionInstance(program, functionInstance.filename + "{err}" + filePosition) != -1){
                            return true;
                        }
                        definition = createFunctionInstance(functionDecl->getDefinition());
                        definition.filename = functionInstance.filename;
                        definition.filePosition = functionInstance.filename + "{err}" + filePosition;
                        definition.declarations[0].filename = definition.filename;
                        definition.declarations[0].filePosition = definition.filePosition;
                    }else{
                        definition = createFunctionInstance(functionDecl->getDefinition());
                    }
                    definition.declarations.push_back(functionInstance);
                    program->push_back(definition);
                }
            }else{

                if(mapOfDeclarations->find(getLocation(functionDecl, dir, Context)) != mapOfDeclarations->end()){
                    return true;
                }
                auto functionInstance = createFunctionInstance(functionDecl);
                mapOfDeclarations->insert(make_pair(getLocation(functionDecl, dir, Context), functionInstance));
            }
        }else{
            if(findFunctionInstance(program, getLocation(functionDecl, dir, Context)) == -1){
                auto functionInstance = createFunctionInstance(functionDecl);
                program->push_back(functionInstance);
            }
        }
        return true;
    };

    bool VisitVarDecl(clang::VarDecl* varDecl){
        // global variables of the files (i.e. not class / struct members)
        if(!varDecl->hasGlobalStorage() && !varDecl->hasExternalStorage()){
            return true;
        }

        auto filename = getVarDeclFilename(varDecl, Context, dir);
        if(std::find(files.begin(), files.end(), filename) == files.end()){
            return true;
        }

        VariableInstance variableInstance;
        if(findVariableInstance(variables, getLocation(varDecl, dir, Context)) != -1){
            return true;
        }
        
        variableInstance = createVariableInstance(varDecl);
        variables->push_back(variableInstance);

        return true;
    }

    bool VisitFieldDecl(clang::FieldDecl* decl){
        auto filename = getVarDeclFilename(decl, Context, dir);
        if(std::find(files.begin(), files.end(), filename) == files.end()){
            return true;
        }

        VariableInstance variableInstance;

        if(findVariableInstance(variables, getLocation(decl, dir, Context)) != -1){
            return true;
        }
        variableInstance = createVariableInstance(decl);
        variables->push_back(variableInstance);

        return true;
    }

private:
    clang::ASTContext *Context;
};


class APIAnalysisConsumer : public clang::ASTConsumer {
public:
    explicit APIAnalysisConsumer(clang::ASTContext *Context, std::vector<FunctionInstance>* program, std::string dir, std::vector<variableanalysis::VariableInstance>* var, std::vector<objectanalysis::ObjectInstance>* objects, std::vector<std::string>& files, std::map<std::string, FunctionInstance>* mapOfDeclarations, std::map<std::string, VariableInstance>* variableDefinitions) : apiAnalysisVisitor(Context, program, dir, var, objects, files, mapOfDeclarations, variableDefinitions){}

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
    std::map<std::string, VariableInstance>* variableDefinitions;
public:
    explicit APIAnalysisAction(std::vector<FunctionInstance>* program, std::string dir, std::vector<variableanalysis::VariableInstance>* var, std::vector<objectanalysis::ObjectInstance>* objects, std::vector<std::string> files, std::map<std::string, FunctionInstance>* mapOfDeclarations, std::map<std::string, VariableInstance>* variableDefinitions){
        this->p=program;
        this->directory = dir;
        this->var = var;
        this->obj = objects;
        this->files = files;
        this->mapOfDeclarations = mapOfDeclarations;
        this->variableDefinitions = variableDefinitions;
    };

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        //Compiler.getDiagnostics().setClient(new clang::IgnoringDiagConsumer(), true);
        return std::make_unique<APIAnalysisConsumer>(&Compiler.getASTContext(), p, directory, var, obj, files, mapOfDeclarations, variableDefinitions);
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
/*
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
            if(otherItem.qualifiedName != item.qualifiedName || item.filename != otherItem.filename){
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
 */

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
            if(!otherItem.isTemplateSpec || otherItem.qualifiedName != item.qualifiedName || item.params.size() != otherItem.params.size() || !FunctionAnalyser::checkIfADeclarationMatches(item, otherItem)){
                continue;
            }
            item.templateSpecializations.push_back(otherItem);
            usedSpecializations.push_back(otherItem);
        }
    }

    // delete all the template specalizations from the list
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
/*
void insertUndeclaredVariables(vector<VariableInstance> *declarations, map<std::string, VariableInstance> *definitions) {
    outs()<<"Started with " << definitions->size() << " definitions\n";
    for (auto &decl : *declarations) {
        for (const auto &def: decl.definitions){
            definitions->erase(def.filePosition);
        }
    }

    outs()<<"There are " << definitions->size() << " undefined definitions\n";
    for (auto &item: *definitions){
        declarations->push_back(item.second);
    }
}
 */

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
            ("ignore-CD-files, iCDf", "Forcefully ignore the files found in the Compilation Database and instead use the files found by manually searching the given folders")
            ("ignore-CD-files-old, iCDfO", "Forcefully ignore the files found in the Compilation Database of the old project and instead use the files found by manually searching the given folders")
            ("ignore-CD-files-new, iCDfN", "Forcefully ignore the files found in the Compilation Database of the new project and instead use the files found by manually searching the given folders")
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

    bool ignoreCDFiles = result["ignore-CD-files"].as<bool>();
    bool ignoreCDFilesOld = result["ignore-CD-files-old"].as<bool>();
    bool ignoreCDFilesNew = result["ignore-CD-files-new"].as<bool>();

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


    std::vector<std::string> relativeListOfOldFiles;
    std::vector<std::string> relativeListOfNewFiles;

    // TODO null check for autodetect CD
    if(result.count("oldCD")){
        std::string errorMessage = "Could not load the specified old compilation Database, trying to find one in the project files\n";
        oldCD = FixedCompilationDatabase::autoDetectFromDirectory(std::filesystem::canonical(result["oldCD"].as<std::string>()).string(), errorMessage);
        if(oldCD && !ignoreCDFilesOld && !ignoreCDFiles){
            oldFiles.clear();
            oldFiles = oldCD->getAllFiles();
            outs()<<oldFiles.size()<<"\n";
            oldFiles = helper::excludeFiles(filesystem::canonical(result["oldDir"].as<std::string>()), oldFiles, &oldExcludedItems);
        }
    }

    if(result.count("newCD")){
        std::string errorMessage = "Could not load the specified new compilation Database, trying to find one in the project files\n";
        newCD = FixedCompilationDatabase::autoDetectFromDirectory(std::filesystem::canonical(result["newCD"].as<std::string>()).string(), errorMessage);
        if(newCD && !ignoreCDFilesNew && !ignoreCDFiles){
            newFiles.clear();
            newFiles = newCD->getAllFiles();
            outs()<<newFiles.size()<<"\n";
            newFiles = helper::excludeFiles(filesystem::canonical(result["newDir"].as<std::string>()), newFiles, &newExcludedItems);
        }
    }

    std::string errorMessage="No Compilation database could be found in the old directory, loading the standard empty compilation database";
    if(!oldCD) {
        oldCD = CompilationDatabase::autoDetectFromDirectory(
                std::filesystem::canonical(result["oldDir"].as<std::string>()).string(), errorMessage);
        if(oldCD && !ignoreCDFilesOld && !ignoreCDFiles){
            oldFiles.clear();
            oldFiles = oldCD->getAllFiles();
            oldFiles = helper::excludeFiles(filesystem::canonical(result["oldDir"].as<std::string>()), oldFiles, &oldExcludedItems);
        }
    }
    if(!newCD) {
        newCD = CompilationDatabase::autoDetectFromDirectory(
                std::filesystem::canonical(result["newDir"].as<std::string>()).string(), errorMessage);
        if(newCD && !ignoreCDFilesNew && !ignoreCDFiles){
            newFiles.clear();
            newFiles = newCD->getAllFiles();
            newFiles = helper::excludeFiles(filesystem::canonical(result["newDir"].as<std::string>()), newFiles, &newExcludedItems);
        }
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

    relativeListOfOldFiles.reserve(oldFiles.size());
    for (const auto &item: oldFiles){
        relativeListOfOldFiles.push_back(std::filesystem::relative(item, std::filesystem::canonical(std::filesystem::absolute(result["oldDir"].as<std::string>()))).string());
    }
    relativeListOfNewFiles.reserve(newFiles.size());
    for (const auto &item: newFiles){
        relativeListOfNewFiles.push_back(std::filesystem::relative(item, std::filesystem::canonical(std::filesystem::absolute(result["newDir"].as<std::string>()))).string());
    }

    std::map<std::string, FunctionInstance> mapOfDeclarations;
    std::map<std::string, VariableInstance> variableDefinitions;

    int fileCounter = 0;
    for (const auto &item: oldFiles){
        fileCounter++;
        if(fileCounter % 100 == 0) outs()<<"Processing file " + itostr(fileCounter) + "/" + itostr(oldFiles.size()) + "\n";
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
        oldTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&oldProgram, std::filesystem::canonical(std::filesystem::absolute(result["oldDir"].as<std::string>())), &oldVariables, &oldObjects, relativeListOfOldFiles, &mapOfDeclarations, &variableDefinitions).get());
    }

    insertUndefinedDeclarations(&oldProgram, &mapOfDeclarations);

    mapOfDeclarations.clear();
    variableDefinitions.clear();
    fileCounter = 0;
    for (const auto &item: newFiles){
        fileCounter++;
        if(fileCounter % 100 == 0) outs()<<"Processing file " + itostr(fileCounter) + "/" + itostr(newFiles.size()) + "\n";
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
        newTool.run(argumentParsingFrontendActionFactory<APIAnalysisAction>(&newProgram, std::filesystem::canonical(std::filesystem::absolute(result["newDir"].as<std::string>())), &newVariables, &newObjects, relativeListOfNewFiles, &mapOfDeclarations, &variableDefinitions).get());
    }

    insertUndefinedDeclarations(&newProgram, &mapOfDeclarations);

    outs()<<"All functions, objects and variables were processed\n";
    outs()<<"In total "<<oldProgram.size()<<" functions, "<<oldObjects.size()<<" objects and "<<oldVariables.size()<<" variables were found in the old version of the project\n";
    outs()<<"In total "<<newProgram.size()<<" functions, "<<newObjects.size()<<" objects and "<<newVariables.size()<<" variables were found in the new version of the project\n";

    oldProgram = assignSpecializations(oldProgram);
    newProgram = assignSpecializations(newProgram);

    OutputHandler* outputHandler;
    if(jsonOutput){
        outputHandler = new JSONOutputHandler();
    }else{
        outputHandler = new ConsoleOutputHandler();
    }
    outs()<<"The objectanalysis started with "<< oldObjects.size() << "objects \n";
    // Analysing Objects
    objectanalysis::ObjectAnalyser objectAnalyser = objectanalysis::ObjectAnalyser(oldObjects, newObjects, outputHandler);
    objectAnalyser.compareObjects();
    outs()<<"The functionanalysis started " << oldProgram.size() << " funcs \n";
    // Analysing Functions
    FunctionAnalyser analyser = FunctionAnalyser(oldProgram, newProgram, outputHandler);
    analyser.compareVersionsWithDoc(docEnabled, outputPrivateFunctions);
    outs()<<"The variableanalysis started " << oldVariables.size() << " variables \n";
    // Analysing Variables
    variableanalysis::VariableAnalyser variableAnalyser = variableanalysis::VariableAnalyser(oldVariables, newVariables, outputHandler);
    variableAnalyser.compareVariables();

    outputHandler->printOut();

    return 0;
}
