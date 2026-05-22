#include "PofBlueprintIntrospector.h"
#include "PillarsOfFortuneBridge.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_Event.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString UPofBlueprintIntrospector::IntrospectBlueprintByPath(const FString& AssetPath)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!Blueprint)
	{
		TSharedRef<FJsonObject> ErrorJson = MakeShareable(new FJsonObject());
		ErrorJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Blueprint not found: %s"), *AssetPath));

		FString Output;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(ErrorJson, Writer);
		return Output;
	}

	FPofBlueprintEntry Entry = IntrospectBlueprint(Blueprint);

	// Serialize to JSON
	TSharedRef<FJsonObject> Json = MakeShareable(new FJsonObject());
	Json->SetStringField(TEXT("path"), Entry.Path);
	Json->SetStringField(TEXT("parentCppClass"), Entry.ParentCppClass);
	Json->SetStringField(TEXT("parentCppModule"), Entry.ParentCppModule);
	Json->SetStringField(TEXT("parentBlueprintClass"), Entry.ParentBlueprintClass);

	// Overridden functions
	TArray<TSharedPtr<FJsonValue>> FuncArray;
	for (const FPofFunctionOverride& Func : Entry.OverriddenFunctions)
	{
		TSharedRef<FJsonObject> FObj = MakeShareable(new FJsonObject());
		FObj->SetStringField(TEXT("functionName"), Func.FunctionName);
		FObj->SetStringField(TEXT("declaringClass"), Func.DeclaringClass);
		FObj->SetBoolField(TEXT("isEvent"), Func.IsEvent);
		FObj->SetBoolField(TEXT("isBlueprintCallable"), Func.IsBlueprintCallable);
		FuncArray.Add(MakeShareable(new FJsonValueObject(FObj)));
	}
	Json->SetArrayField(TEXT("overriddenFunctions"), FuncArray);

	// Components
	TArray<TSharedPtr<FJsonValue>> CompArray;
	for (const FPofComponentEntry& Comp : Entry.AddedComponents)
	{
		TSharedRef<FJsonObject> CObj = MakeShareable(new FJsonObject());
		CObj->SetStringField(TEXT("componentName"), Comp.ComponentName);
		CObj->SetStringField(TEXT("componentClass"), Comp.ComponentClass);
		CObj->SetBoolField(TEXT("isSceneComponent"), Comp.IsSceneComponent);
		CObj->SetStringField(TEXT("attachParent"), Comp.AttachParent);
		CompArray.Add(MakeShareable(new FJsonValueObject(CObj)));
	}
	Json->SetArrayField(TEXT("addedComponents"), CompArray);

	// Variables
	TArray<TSharedPtr<FJsonValue>> VarArray;
	for (const FPofVariableEntry& Var : Entry.Variables)
	{
		TSharedRef<FJsonObject> VObj = MakeShareable(new FJsonObject());
		VObj->SetStringField(TEXT("name"), Var.Name);
		VObj->SetStringField(TEXT("type"), Var.Type);
		VObj->SetStringField(TEXT("subType"), Var.SubType);
		VObj->SetStringField(TEXT("defaultValue"), Var.DefaultValue);
		VObj->SetBoolField(TEXT("isReplicated"), Var.IsReplicated);
		VObj->SetStringField(TEXT("category"), Var.Category);
		VarArray.Add(MakeShareable(new FJsonValueObject(VObj)));
	}
	Json->SetArrayField(TEXT("variables"), VarArray);

	// Event entry points
	TArray<TSharedPtr<FJsonValue>> EventArray;
	for (const FString& Event : Entry.EventGraphEntryPoints)
	{
		EventArray.Add(MakeShareable(new FJsonValueString(Event)));
	}
	Json->SetArrayField(TEXT("eventGraphEntryPoints"), EventArray);

	// Interfaces
	TArray<TSharedPtr<FJsonValue>> InterfaceArray;
	for (const FString& Interface : Entry.Interfaces)
	{
		InterfaceArray.Add(MakeShareable(new FJsonValueString(Interface)));
	}
	Json->SetArrayField(TEXT("interfaces"), InterfaceArray);

	Json->SetStringField(TEXT("contentHash"), Entry.ContentHash);

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Json, Writer);
	return Output;
}

FPofBlueprintEntry UPofBlueprintIntrospector::IntrospectBlueprint(UBlueprint* Blueprint)
{
	FPofBlueprintEntry Entry;
	Entry.Path = Blueprint->GetPathName();

	// Parent C++ class
	if (UClass* ParentClass = Blueprint->ParentClass)
	{
		UClass* NativeParent = ParentClass;
		while (NativeParent && !NativeParent->IsNative())
		{
			NativeParent = NativeParent->GetSuperClass();
		}

		Entry.ParentCppClass = NativeParent ? NativeParent->GetName() : TEXT("None");
		Entry.ParentCppModule = NativeParent ? NativeParent->GetOuterUPackage()->GetName() : TEXT("");
		Entry.ParentBlueprintClass = (!ParentClass->IsNative()) ? ParentClass->GetName() : TEXT("");
	}

	// Overridden functions
	UClass* GeneratedClass = Blueprint->GeneratedClass;
	if (GeneratedClass)
	{
		for (TFieldIterator<UFunction> FuncIt(GeneratedClass, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
		{
			UFunction* Func = *FuncIt;
			UFunction* SuperFunc = GeneratedClass->GetSuperClass()
				? GeneratedClass->GetSuperClass()->FindFunctionByName(Func->GetFName())
				: nullptr;

			if (SuperFunc && SuperFunc->IsNative())
			{
				FPofFunctionOverride Override;
				Override.FunctionName = Func->GetName();
				Override.DeclaringClass = SuperFunc->GetOwnerClass()->GetName();
				Override.IsEvent = Func->HasAnyFunctionFlags(FUNC_Event);
				Override.IsBlueprintCallable = Func->HasAnyFunctionFlags(FUNC_BlueprintCallable);
				Entry.OverriddenFunctions.Add(Override);
			}
		}
	}

	// Added components
	if (Blueprint->SimpleConstructionScript)
	{
		TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
		for (USCS_Node* Node : AllNodes)
		{
			if (Node && Node->ComponentClass)
			{
				FPofComponentEntry Comp;
				Comp.ComponentName = Node->GetVariableName().ToString();
				Comp.ComponentClass = Node->ComponentClass->GetName();
				Comp.IsSceneComponent = Node->ComponentClass->IsChildOf(USceneComponent::StaticClass());

				if (Node->ParentComponentOrVariableName != NAME_None)
				{
					Comp.AttachParent = Node->ParentComponentOrVariableName.ToString();
				}

				Entry.AddedComponents.Add(Comp);
			}
		}
	}

	// Event graph entry points
	for (UEdGraph* Graph : Blueprint->EventGraphs)
	{
		if (!Graph) continue;
		for (UEdGraphNode* GraphNode : Graph->Nodes)
		{
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(GraphNode))
			{
				Entry.EventGraphEntryPoints.Add(EventNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			}
		}
	}

	// Variables
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		FPofVariableEntry VarEntry;
		VarEntry.Name = Var.VarName.ToString();
		VarEntry.Type = Var.VarType.PinCategory.ToString();
		if (Var.VarType.PinSubCategoryObject.IsValid())
		{
			VarEntry.SubType = Var.VarType.PinSubCategoryObject->GetName();
		}
		VarEntry.DefaultValue = Var.DefaultValue;
		VarEntry.Category = Var.Category.ToString();
		Entry.Variables.Add(VarEntry);
	}

	// Interfaces
	for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
	{
		if (Interface.Interface)
		{
			Entry.Interfaces.Add(Interface.Interface->GetName());
		}
	}

	return Entry;
}
