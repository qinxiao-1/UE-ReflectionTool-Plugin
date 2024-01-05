// Fill out your copyright notice in the Description page of Project Settings.


#include "ReflectionToolLib.h"

#include "DataTableUtils.h"
#include "JsonObjectConverter.h"
#include "StructDeserializer.h"
#include "Backends/JsonStructDeserializerBackend.h"
#include "UObject/UnrealTypePrivate.h"

DEFINE_LOG_CATEGORY(ReflectionTool)

// 解析 InPropertyParserStruct
#define PARSERINPROPERTYPARSERSTRUCT	\
TMap<FString, FPropertyParserStruct> Name2Value;	\
for (auto Child : InPropertyParserStruct.Children)	\
{	\
	Name2Value.Add(Child.Name, Child);	\
}
#define TypeName_TArray L"TArray"
#define TypeName_TSet L"TSet"
#define TypeName_TMap L"TMap"

void UReflectionToolLib::GetStructProperty(const UStruct* StructClass, const void* Struct,
	FPropertyParserStruct& OutPropertyParserStruct)
{
	OutPropertyParserStruct.TypeName = TEXT("Struct");
	OutPropertyParserStruct.bHaveChild = true;
	for (FProperty* Property = StructClass->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		FPropertyParserStruct PropertyParserStruct;
		const void* Addr = Property->ContainerPtrToValuePtr<uint8>(Struct);
		PropertyToPropertyStruct(Property, Addr, PropertyParserStruct);
		OutPropertyParserStruct.Children.Add(PropertyParserStruct);
	}
}

void UReflectionToolLib::StructToPropertyStruct(FStructProperty* StructProperty, const void* Addr,
	FPropertyParserStruct& OutPropertyParserStruct)
{
	OutPropertyParserStruct.Name = StructProperty->GetAuthoredName();
	// OutPropertyParserStruct.TypeName = TEXT("Struct");
	OutPropertyParserStruct.bHaveChild = true;
	for (FProperty* Property = StructProperty->Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		FPropertyParserStruct PropertyParserStruct;
		const void* NewAddr = Property->ContainerPtrToValuePtr<uint8>(Addr);
		PropertyToPropertyStruct(Property, NewAddr, PropertyParserStruct);
		OutPropertyParserStruct.Children.Add(PropertyParserStruct);
	}
}

void UReflectionToolLib::TArrayToPropertyStruct(FArrayProperty* ArrayProperty, const void* Addr,
                                                      FPropertyParserStruct& OutPropertyParserStruct)
{
	OutPropertyParserStruct.TypeName = TypeName_TArray;
	OutPropertyParserStruct.bHaveChild = true;
	FScriptArrayHelper Helper(ArrayProperty, Addr);
	for (int i = 0, n = Helper.Num(); i < n; ++i)
	{
		FPropertyParserStruct PropertyParserStruct;
		PropertyToPropertyStruct(ArrayProperty->Inner, Helper.GetRawPtr(i), PropertyParserStruct);
		OutPropertyParserStruct.Children.Add(PropertyParserStruct);
	}
}

void UReflectionToolLib::TSetToPropertyStruct(FSetProperty* SetProperty, const void* Addr,
	FPropertyParserStruct& OutPropertyParserStruct)
{
	OutPropertyParserStruct.TypeName = TypeName_TSet;
	OutPropertyParserStruct.bHaveChild = true;
	FScriptSetHelper Helper(SetProperty, Addr);
	for (int i = 0, n = Helper.Num(); n; ++i)
	{
		if (Helper.IsValidIndex(i))
		{
			FPropertyParserStruct PropertyParserStruct;
			PropertyToPropertyStruct(SetProperty->ElementProp, Helper.GetElementPtr(i), PropertyParserStruct);
			OutPropertyParserStruct.Children.Add(PropertyParserStruct);
			--n;
		}
	}
}

void UReflectionToolLib::TMapToPropertyStruct(FMapProperty* MapProperty, const void* Addr,
	FPropertyParserStruct& OutPropertyParserStruct)
{
	OutPropertyParserStruct.TypeName = TypeName_TMap;
	OutPropertyParserStruct.bHaveChild = true;
	FScriptMapHelper Helper(MapProperty, Addr);
	for (int i = 0, n = Helper.Num(); n; ++i)
	{
		if (Helper.IsValidIndex(i))
		{
			FPropertyParserStruct KeyPropertyParserStruct, ValuePropertyParserStruct;
			PropertyToPropertyStruct(MapProperty->KeyProp, Helper.GetKeyPtr(i), KeyPropertyParserStruct);
			PropertyToPropertyStruct(MapProperty->ValueProp, Helper.GetValuePtr(i), ValuePropertyParserStruct);
			FPropertyParserStruct MapItemPropertyParserStruct;
			MapItemPropertyParserStruct.Name = FString::FromInt(i);
			MapItemPropertyParserStruct.TypeName = TEXT("MapItem");
			MapItemPropertyParserStruct.bHaveChild = true;
			MapItemPropertyParserStruct.Children.Add(KeyPropertyParserStruct);
			MapItemPropertyParserStruct.Children.Add(ValuePropertyParserStruct);
			OutPropertyParserStruct.Children.Add(MapItemPropertyParserStruct);
			--n;
		}
	}
	
}

void UReflectionToolLib::PropertyToPropertyStruct(FProperty* Property, const void* Addr,
                                                        FPropertyParserStruct& OutPropertyParserStruct)
{
	OutPropertyParserStruct.Name = Property->GetAuthoredName();
	if (Property->GetCPPType().Contains(TEXT("<")))
	{
		Property->GetCPPType().Split(TEXT("<"), &OutPropertyParserStruct.TypeName, nullptr);
	}
	else
	{
		OutPropertyParserStruct.TypeName = Property->GetCPPType();
	}
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		const UEnum* EnumDef = EnumProperty->GetEnum();
		OutPropertyParserStruct.Value = EnumDef->GetAuthoredNameStringByValue(EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Addr));
	}
	else if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		if (UEnum* EnumDef = NumericProperty->GetIntPropertyEnum(); EnumDef != NULL)
		{
			OutPropertyParserStruct.Value = EnumDef->GetAuthoredNameStringByValue(NumericProperty->GetSignedIntPropertyValue(Addr));
		}
		if (NumericProperty->IsFloatingPoint())
		{
			OutPropertyParserStruct.Value = FString::Printf(TEXT("%.15f"), NumericProperty->GetFloatingPointPropertyValue(Addr));
		}
		else if (NumericProperty->IsInteger())
		{
			OutPropertyParserStruct.Value = FString::Printf(TEXT("%lld"), NumericProperty->GetSignedIntPropertyValue(Addr));
		}
	}
	else if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		OutPropertyParserStruct.Value = BoolProperty->GetPropertyValue(Addr) ? TEXT("true") : TEXT("false");
	}
	else if (const FStrProperty* StringProperty = CastField<FStrProperty>(Property))
	{
		OutPropertyParserStruct.Value = StringProperty->GetPropertyValue(Addr);
	}
	else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		OutPropertyParserStruct.Value = NameProperty->GetPropertyValue(Addr).ToString();
	}
	else if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		OutPropertyParserStruct.Value = TextProperty->GetPropertyValue(Addr).ToString();
	}
	else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		StructToPropertyStruct(StructProperty, Addr, OutPropertyParserStruct);
	}
	else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		TArrayToPropertyStruct(ArrayProperty, Addr, OutPropertyParserStruct);
	}
	else if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		TSetToPropertyStruct(SetProperty, Addr, OutPropertyParserStruct);
	}
	else if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		TMapToPropertyStruct(MapProperty, Addr, OutPropertyParserStruct);
	}
	else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		if (UObject* Object = ObjectProperty->GetObjectPropertyValue(Addr))
		{
			OutPropertyParserStruct.TypeName = OutPropertyParserStruct.TypeName.Replace(TEXT("*"), TEXT("_Ptr"));
			OutPropertyParserStruct.Value = ObjectProperty->GetObjectPropertyValue(Addr)->GetPathName();
		}
	}
	else
	{
		Property->ExportTextItem_Direct(OutPropertyParserStruct.Value, Addr, NULL, NULL, PPF_None);
	}
}

void UReflectionToolLib::ParserPropertyParserStruct(const UStruct* StructClass, void* Struct,
	FPropertyParserStruct& InPropertyParserStruct)
{
	PARSERINPROPERTYPARSERSTRUCT
	for (FProperty* Property = StructClass->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		FPropertyParserStruct PropertyParserStruct;
		void* Addr = Property->ContainerPtrToValuePtr<uint8>(Struct);
		
		const FString TempName = Property->GetAuthoredName();
		if (FPropertyParserStruct* TargetPPS = Name2Value.Find(TempName))
		{
			// 开始对各个值的处理
			ParserPPSToProperty(Property, Addr, *TargetPPS);
		}
	}
}

void UReflectionToolLib::ParserPPSToArrayProperty(FArrayProperty* ArrayProperty, void* Addr,
	FPropertyParserStruct& InPropertyParserStruct)
{
	const int Len =  InPropertyParserStruct.Children.Num();
	FScriptArrayHelper Helper(ArrayProperty, Addr);
	Helper.Resize(Len);
	for (int i = 0; i < Len; ++i)
	{
		// 设置给 TArray
		ParserPPSToProperty(ArrayProperty->Inner, Helper.GetRawPtr(i), InPropertyParserStruct.Children[i]);
	}
}

void UReflectionToolLib::ParserPPSToSetProperty(FSetProperty* SetProperty, void* Addr,
	FPropertyParserStruct& InPropertyParserStruct)
{
	FScriptSetHelper Helper(SetProperty, Addr);
	for	(int i = 0, Len =  InPropertyParserStruct.Children.Num(); i < Len; ++i)
	{
		const int32 NewIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();
		ParserPPSToProperty(SetProperty->ElementProp, Helper.GetElementPtr(NewIndex), InPropertyParserStruct.Children[i]);
	}
}

void UReflectionToolLib::ParserPPSToMapProperty(FMapProperty* MapProperty, void* Addr,
	FPropertyParserStruct& InPropertyParserStruct)
{
	FScriptMapHelper Helper(MapProperty, Addr);
	for	(int i = 0, Len =  InPropertyParserStruct.Children.Num(); i < Len; ++i)
	{
		const int32 NewIndex = Helper.AddDefaultValue_Invalid_NeedsRehash();
		ParserPPSToProperty(MapProperty->KeyProp, Helper.GetKeyPtr(NewIndex), InPropertyParserStruct.Children[i].Children[0]);
		ParserPPSToProperty(MapProperty->ValueProp, Helper.GetValuePtr(NewIndex), InPropertyParserStruct.Children[i].Children[1]);
	}
	Helper.Rehash();
}

void UReflectionToolLib::ParserPPSToStructProperty(FStructProperty* StructProperty, void* Addr,
	FPropertyParserStruct& InPropertyParserStruct)
{
	// 解析
	PARSERINPROPERTYPARSERSTRUCT
	for (FProperty* Property = StructProperty->Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		FPropertyParserStruct PropertyParserStruct;
		void* NewAddr = Property->ContainerPtrToValuePtr<uint8>(Addr);
		
		// 开始对各个值的处理
		const FString TempName = Property->GetAuthoredName();
		if (FPropertyParserStruct* TargetPPS = Name2Value.Find(TempName))
		{
			ParserPPSToProperty(Property, NewAddr, *TargetPPS);
		}
	}
}

void UReflectionToolLib::SetFStringToEnumProperty(FEnumProperty* EnumProperty, void* Addr, const FString& EnumString)
{
	TMap<FName, int> TempTMap;
	const UEnum* EnumClass = EnumProperty->GetEnum();
	for (int i = 0; i < EnumClass->NumEnums(); ++i)
	{
		TempTMap.Add(EnumClass->GetNameByIndex(i), EnumClass->GetValueByIndex(i));
	}
	const FByteProperty* ByteProperty = CastField<FByteProperty>(EnumProperty);
	const FName TempEnumString = FName(EnumClass->GetName() + "::" + EnumString); 
	if (TempTMap.Contains(TempEnumString))
	{		
		ByteProperty->SetPropertyValue(Addr, TempTMap[TempEnumString]);
	}
}

void UReflectionToolLib::ParserPPSToProperty(FProperty* Property, void* Addr,
	FPropertyParserStruct& OutPropertyParserStruct)
{
	// 区分简单 / 复杂
	if (OutPropertyParserStruct.bHaveChild)
	{
		ParserComplexPPSToProperty(Property, Addr, OutPropertyParserStruct);
	}
	else
	{
		ParserSinglePPSToProperty(Property, Addr, OutPropertyParserStruct);
	}
}

void UReflectionToolLib::ParserSinglePPSToProperty(FProperty* Property, void* Addr,
                                                         FPropertyParserStruct& InPropertyParserStruct)
{
	if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		SetFStringToEnumProperty(EnumProperty, Addr, InPropertyParserStruct.Value);
	}
	else if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		if (UEnum* EnumDef = NumericProperty->GetIntPropertyEnum(); EnumDef != NULL)
		{
			SetFStringToEnumProperty(EnumProperty, Addr, InPropertyParserStruct.Value);
		}
		if (NumericProperty->IsFloatingPoint())
		{
			const double temp = FCString::Atod(*InPropertyParserStruct.Value);
			NumericProperty->SetFloatingPointPropertyValue(Addr, temp);
		}
		else if (NumericProperty->IsInteger())
		{
			const int64 temp = FCString::Atoi64(*InPropertyParserStruct.Value);
			NumericProperty->SetIntPropertyValue(Addr, temp);
		}
	}
	else if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		const bool temp = InPropertyParserStruct.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
		BoolProperty->SetPropertyValue(Addr, temp);
	}
	else if (const FStrProperty* StringProperty = CastField<FStrProperty>(Property))
	{
		StringProperty->SetPropertyValue(Addr, InPropertyParserStruct.Value);
	}
	else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		NameProperty->SetPropertyValue(Addr, FName(InPropertyParserStruct.Value));
	}
	else if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		TextProperty->SetPropertyValue(Addr, FText::FromString(InPropertyParserStruct.Value));
	}
	else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		UObject* Object = StaticLoadObject(UObject::StaticClass(), nullptr, *InPropertyParserStruct.Value);
		ObjectProperty->SetObjectPropertyValue(Addr, Object);
	}
	else
	{
		const TCHAR* ImportTextPtr = *InPropertyParserStruct.Value;
		// Property->ImportText(ImportTextPtr, Addr, PPF_None, nullptr);
		Property->ImportText_Direct(ImportTextPtr, Addr, nullptr, PPF_None);
	}
}

void UReflectionToolLib::ParserComplexPPSToProperty(FProperty* Property, void* Addr,
	FPropertyParserStruct& InPropertyParserStruct)
{
	if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		ParserPPSToArrayProperty(ArrayProperty, Addr, InPropertyParserStruct);
	}
	else if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		ParserPPSToSetProperty(SetProperty, Addr, InPropertyParserStruct);
	}
	else if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		ParserPPSToMapProperty(MapProperty, Addr, InPropertyParserStruct);
	}
	else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		ParserPPSToStructProperty(StructProperty, Addr, InPropertyParserStruct);
	}
}

#if WITH_EDITOR

bool UReflectionToolLib::GetFunctionsByCategories(UClass* Class, const TArray<FString>& Categories,
	TArray<FFunctionInfo>& Results, bool ShowLog)
{
	if (!Class)
		return false;
	Results.Empty();
	for (TFieldIterator<UFunction> i(Class); i; ++i)
	{
		FFunctionInfo result;
		UFunction* func = *i;
		FString funcCategory = func->GetMetaData("Category");
		if (!Categories.Contains(funcCategory))
			continue;
		FString funcName = func->GetName();
		FString funcDesc = func->GetMetaData("Desc");
		result.FunctionName = funcName;
		result.FunctionDesc = funcDesc;
		if (ShowLog)
		{
			UE_LOG(ReflectionTool, Log, TEXT("--------------------------------"));
			UE_LOG(ReflectionTool, Log, TEXT("FuncCategory is : %s"), *funcCategory);
			UE_LOG(ReflectionTool, Log, TEXT("FuncName is : %s"), *funcName);
			UE_LOG(ReflectionTool, Log, TEXT("FuncDesc is : %s"), *funcDesc);
		}
		TArray<FFuncParameter> InParams = TArray<FFuncParameter>({});
		TArray<FFuncParameter> OutParams = TArray<FFuncParameter>({});
		for	(const FProperty* Property = func->PropertyLink; Property; Property = Property->PropertyLinkNext)
		{
			if (Property->PropertyFlags & CPF_OutParm)
			{
				OutParams.Add(FFuncParameter(Property->GetCPPType(), Property->GetName()));
				if (ShowLog)
					UE_LOG(ReflectionTool, Log, TEXT("Out Param Name : [%s], Type is [%s]"), *Property->GetName(), *Property->GetCPPType());	
			}
			else
			{
				InParams.Add(FFuncParameter(Property->GetCPPType(), Property->GetName()));
				if (ShowLog)
					UE_LOG(ReflectionTool, Log, TEXT("Param Name : [%s], Type is [%s]"), *Property->GetName(), *Property->GetCPPType());
			}
		}
		if (ShowLog)
			UE_LOG(LogTemp, Warning, TEXT("--------------------------------\n"));
		result.InParams = InParams;
		result.OutParams = OutParams;
		Results.Add(result);
	}
	return true;
}

bool UReflectionToolLib::GetFunctionsBySuffix(UClass* Class, const TArray<FString>& Suffix,
	TArray<FFunctionInfo>& Results, bool ShowLog)
{
	if (!Class)
		return false;
	Results.Empty();
	for (TFieldIterator<UFunction> i(Class); i; ++i)
	{
		FFunctionInfo result;
		UFunction* func = *i;
		FString funcName = func->GetName();
		bool tempShouldContinue = !Suffix.IsEmpty();
		for (auto eachSuffix : Suffix)
		{
			if (funcName.EndsWith(eachSuffix))
			{
				tempShouldContinue = false;
				break;
			}
		}
		if (tempShouldContinue)
			continue;;
		FString funcDesc = func->GetMetaData("Desc");
		result.FunctionName = funcName;
		result.FunctionDesc = funcDesc;
		if (ShowLog)
		{
			UE_LOG(ReflectionTool, Log, TEXT("--------------------------------"));
			UE_LOG(ReflectionTool, Log, TEXT("FuncName is : %s"), *funcName);
			UE_LOG(ReflectionTool, Log, TEXT("FuncDesc is : %s"), *funcDesc);
		}
		TArray<FFuncParameter> InParams = TArray<FFuncParameter>({});
		TArray<FFuncParameter> OutParams = TArray<FFuncParameter>({});
		for	(const FProperty* Property = func->PropertyLink; Property; Property = Property->PropertyLinkNext)
		{
			if (Property->PropertyFlags & CPF_OutParm)
			{
				OutParams.Add(FFuncParameter(Property->GetCPPType(), Property->GetName()));
				if (ShowLog)
					UE_LOG(ReflectionTool, Log, TEXT("Out Param Name : [%s], Type is [%s]"), *Property->GetName(), *Property->GetCPPType());	
			}
			else
			{
				InParams.Add(FFuncParameter(Property->GetCPPType(), Property->GetName()));
				if (ShowLog)
					UE_LOG(ReflectionTool, Log, TEXT("Param Name : [%s], Type is [%s]"), *Property->GetName(), *Property->GetCPPType());
			}
		}
		if (ShowLog)
			UE_LOG(LogTemp, Warning, TEXT("--------------------------------\n"));
		result.InParams = InParams;
		result.OutParams = OutParams;
		Results.Add(result);
	}
	return true;
}

bool UReflectionToolLib::GetProperitesByCategories(UClass* Class, const TArray<FString>& Categories,
	TArray<FFuncParameter>& Results, bool ShowLog)
{
	if (!Class)
		return false;
	Results.Empty();
	for (TFieldIterator<FProperty> i(Class); i; ++i)
	{
		FFunctionInfo result;
		FProperty* property = *i;
		FString propCategory = property->GetMetaData("Category");
		bool shouldContinue = true;
		for (auto eachTargetCategory : Categories)
		{
			if (propCategory.Contains(eachTargetCategory))
			{
				shouldContinue = false;
				break;
			}
		}
		if (shouldContinue)
			continue;
		Results.Emplace(FFuncParameter(property->GetCPPType(), property->GetName()));
	}
	return true;
}

#endif

bool UReflectionToolLib::InvokeFunctionByName_Map(UObject* TargetObject, const FName& FunctionName,
	const TMap<FString, FString>& InParams, TMap<FString, FString>& OutParams)
{
	if (!TargetObject)
		return false;
	UFunction* Func = TargetObject->GetClass()->FindFunctionByName(FunctionName);
	if (!Func)
		return false;
	return InvokeFunctionByName(TargetObject->GetClass(), TargetObject, Func, InParams, OutParams);
}

bool UReflectionToolLib::InvokeFunctionByName(UClass* Class, UObject* TargetObject, UFunction* Function,
                                              const TMap<FString, FString>& InParams, TMap<FString, FString>& OutParams)
{
	// 调用静态函数不需要对象
	Class = TargetObject == nullptr ? Class : TargetObject->GetClass();
	UObject* Context = TargetObject == nullptr ? Class : TargetObject;

	if (!Function)
		return false;
	
	TArray<uint8> Params;
	Params.AddUninitialized(Function->ParmsSize);
	if (Function->ParmsSize > 0)
	{
		Function->InitializeStruct(Params.GetData());

		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();

		for (auto Item : InParams)
		{
			FProperty* Property = Function->FindPropertyByName(FName(*Item.Key));
			SetJsonFieldByProperty(JsonObject, Property, Item.Key, Item.Value);
		}

		TArray<uint8> FunctionParamsContainer;
		FMemoryWriter Writer(FunctionParamsContainer);
		TSharedRef<TJsonWriter<UCS2CHAR>> JsonWriter = TJsonWriter<UCS2CHAR>::Create(&Writer);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
		FMemoryReader Reader(FunctionParamsContainer);
		FJsonStructDeserializerBackend DeserializerBackend(Reader);
		FStructDeserializer::Deserialize(Params.GetData(), *Function, DeserializerBackend);
	}

	Context->ProcessEvent(Function, Params.GetData());

	for (auto Item : OutParams)
	{
		FProperty* Property = Function->FindPropertyByName(FName(*Item.Key));
		void* Addr = Property->ContainerPtrToValuePtr<uint8>(Params.GetData());
		FPropertyParserStruct outPPS;
		PropertyToPropertyStruct(Property, Addr, outPPS);
		OutParams[Item.Key] = outPPS.Value;
	}

	return true;
}

bool UReflectionToolLib::InvokeFunctionByName_Array(UObject* TargetObject, const FName& FunctionName,
	const TArray<FString>& InParams)
{
	if (!TargetObject)
		return false;
	UFunction* Func = TargetObject->GetClass()->FindFunctionByName(FunctionName);
	if (!Func)
		return false;
	return InvokeFunctionByName(TargetObject->GetClass(), TargetObject, Func, InParams);
}

bool UReflectionToolLib::InvokeFunctionByName(UClass* Class, UObject* TargetObject, UFunction* Function,
                                              const TArray<FString>& InParams)
{
	// 调用静态函数不需要对象
	Class = TargetObject == nullptr ? Class : TargetObject->GetClass();
	UObject* Context = TargetObject == nullptr ? Class : TargetObject;

	if (!Function)
		return false;
	
	TArray<uint8> Params;
	Params.AddUninitialized(Function->ParmsSize);
	if (Function->ParmsSize > 0)
	{
		Function->InitializeStruct(Params.GetData());

		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		int32 inIndex = 0;

		for	(FProperty* Property = Function->PropertyLink; Property && inIndex < InParams.Num(); Property = Property->PropertyLinkNext)
		{
			if (Property->PropertyFlags & CPF_Parm)
			{
				SetJsonFieldByProperty(JsonObject, Property, Property->GetName(), InParams[inIndex]);
				++inIndex;
			}
		}

		TArray<uint8> FunctionParamsContainer;
		FMemoryWriter Writer(FunctionParamsContainer);
		TSharedRef<TJsonWriter<UCS2CHAR>> JsonWriter = TJsonWriter<UCS2CHAR>::Create(&Writer);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
		FMemoryReader Reader(FunctionParamsContainer);
		FJsonStructDeserializerBackend DeserializerBackend(Reader);
		FStructDeserializer::Deserialize(Params.GetData(), *Function, DeserializerBackend);		
	}

	Context->ProcessEvent(Function, Params.GetData());
	return true;
}

void UReflectionToolLib::SetStructValueByMap(const UStruct* StructClass, void* Struct,
                                             const TMap<FString, FString>& InMap)
{
	for (TFieldIterator<FProperty> i(StructClass); i; ++i)
	{
		FProperty* Property = *i;
		void* Addr = Property->ContainerPtrToValuePtr<uint8>(Struct);
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			// 处理结构体类型
			SetStructValueByMap(StructProperty, Addr, InMap);
		}
		else
		{
			// 单值数据处理
			if (const FString* FoundString = InMap.Find(Property->GetAuthoredName()))
			{
				FPropertyParserStruct PropertyParserStruct;
				PropertyParserStruct.Value = *FoundString;
				ParserSinglePPSToProperty(Property, Addr, PropertyParserStruct);
			}
		}
	}
}

void UReflectionToolLib::SetStructValueByMap(FStructProperty* StructProperty, void* Addr,
	const TMap<FString, FString>& InMap)
{
	for (FProperty* Property = StructProperty->Struct->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		void* NewAddr = Property->ContainerPtrToValuePtr<uint8>(Addr);
		if (FStructProperty* tempStructProperty = CastField<FStructProperty>(Property))
		{
			// 处理结构体类型
			SetStructValueByMap(tempStructProperty, NewAddr, InMap);
		}
		else
		{
			// 单值数据处理
			if (const FString* FoundString = InMap.Find(Property->GetAuthoredName()))
			{
				FPropertyParserStruct PropertyParserStruct;
				PropertyParserStruct.Value = *FoundString;
				ParserSinglePPSToProperty(Property, NewAddr, PropertyParserStruct);
			}
		}
	}
}

TArray<FPropertyParserStruct> UReflectionToolLib::GetPPSChildren(const FPropertyParserStruct& PPS)
{
	return PPS.Children;
}

void UReflectionToolLib::AddPPSChild(FPropertyParserStruct& TargetPPS, const FPropertyParserStruct& ChildPPs)
{
	TargetPPS.Children.Add(ChildPPs);
	TargetPPS.bHaveChild = true;
}

void UReflectionToolLib::GetKeyValueFromPPS(const FPropertyParserStruct& PPS, TMap<FString, FString>& ResultMap)
{
	if (PPS.bHaveChild)
	{
		// 目前无法处理存放复杂数据的 TArray TSet TMap，只处理简单数据的处理起来也很怪
		if (PPS.TypeName == TypeName_TArray || PPS.TypeName == TypeName_TSet)
		{
			for (int index = 0; index < PPS.Children.Num(); ++index)
			{
				FPropertyParserStruct ChildPPS = PPS.Children[index];
				if (!ChildPPS.bHaveChild)
				{
					ResultMap.Emplace(FString::Printf(TEXT("%s_%d"), *PPS.Children[index].Name, index),
						PPS.Children[index].Value);
				}
			}
		}
		else if (PPS.TypeName == TypeName_TMap)
		{
			for (int index = 0; index < PPS.Children.Num(); ++index)
			{
				FPropertyParserStruct ChildPPS = PPS.Children[index];
				if (ChildPPS.Children.Num() == 2)
				{
					ResultMap.Emplace(*FString::Printf(TEXT("%s_%s"), *PPS.Name, *ChildPPS.Children[0].Value),
						ChildPPS.Children[1].Value);
				}
			}
		}
		else
		{
			for (auto PPSChild : PPS.Children)
			{
				GetKeyValueFromPPS(PPSChild, ResultMap);
			}
		}
	}
	else
	{
		ResultMap.Emplace(PPS.Name, PPS.Value);
	}
}

void UReflectionToolLib::GetPropertyParserStruct(const int32& StructReference, FPropertyParserStruct& ResMap)
{
	check(0);
}

void UReflectionToolLib::FGetPropertyParserStruct(const void* StructAddr, const UStruct* StructProperty,
	FPropertyParserStruct& OutPropertyParserStruct)
{
	OutPropertyParserStruct.TypeName = TEXT("Struct");
	OutPropertyParserStruct.bHaveChild = true;
	for (TFieldIterator<FProperty> i(StructProperty); i; ++i)
    {
    	FProperty* Property = *i;
		FPropertyParserStruct PropertyParserStruct;
		const void* Addr = Property->ContainerPtrToValuePtr<uint8>(StructAddr);
		PropertyToPropertyStruct(Property, Addr, PropertyParserStruct);
		OutPropertyParserStruct.Children.Add(PropertyParserStruct);
    }
}

void UReflectionToolLib::SetStructByPPS(const int32& StructReference,
	const FPropertyParserStruct& InPropertyParserStruct)
{
	check(0);
}

void UReflectionToolLib::FSetStructByPPS(void* StructAddr, const UStruct* StructProperty,
	FPropertyParserStruct& InPropertyParserStruct)
{
	ParserPropertyParserStruct(StructProperty, StructAddr, InPropertyParserStruct);
}

void UReflectionToolLib::GetStructPropertyMap(const int32& StructReference, TMap<FString, FString>& ResMap)
{
	check(0);
}

void UReflectionToolLib::FGetStructPropertyMap(const void* StructAddr, const UStruct* StructProperty, void* MapAddr,
	FMapProperty* MapProperty)
{
	if (!StructAddr || !MapAddr)
		return;
	FPropertyParserStruct OutPropertyParserStruct;
	FGetPropertyParserStruct(StructAddr, StructProperty, OutPropertyParserStruct);
	TMap<FString, FString> ResultMap;
	GetKeyValueFromPPS(OutPropertyParserStruct, ResultMap);
	
	FScriptMapHelper MapHelper(MapProperty, MapAddr);
	for (auto Item : ResultMap)
	{
		MapHelper.AddPair(&Item.Key, &Item.Value);
	}
}

void UReflectionToolLib::SetStructPropertyByMap(const int32& StructReference, const TMap<FString, FString>& InMap)
{
	check(0);
}

void UReflectionToolLib::FSetStructPropertyByMap(void* StructAddr, UStruct* StructProperty, const void* MapAddr,
	const FMapProperty* MapProperty)
{
	// @XXX
	// Create Map
	TMap<FString, FString> InMap;
	FScriptMapHelper MapHelper(MapProperty, MapAddr);
	for (int index = 0; index < MapHelper.GetMaxIndex(); ++index)
	{
		if (MapHelper.IsValidIndex(index))
		{
			uint8* MapKeyData = MapHelper.GetKeyPtr(index);
			if (MapKeyData)
			{
				const FString Key = *DataTableUtils::GetPropertyValueAsStringDirect(
					MapHelper.GetKeyProperty(), (uint8*)MapKeyData, EDataTableExportFlags::None);
				uint8* MapValueData = MapHelper.GetValuePtr(index);
				FString Value;
				if (FString* tempValue = reinterpret_cast<FString*>(MapValueData))
				{
					Value = *tempValue;
				}
				InMap.Emplace(Key, Value);
			}
		}
	}
	// Map To Struct
	SetStructValueByMap(StructProperty, StructAddr, InMap);
}

void UReflectionToolLib::SetPPSChildren(FPropertyParserStruct& PPS, const TArray<FPropertyParserStruct>& PPSChildren)
{
	PPS.Children = PPSChildren;
}

void UReflectionToolLib::SetJsonFieldByProperty(TSharedPtr<FJsonObject> JsonObject, FProperty* Property,
                                                const FString& Key, const FString& Value)
{
	if (CastField<FNumericProperty>(Property))
	{
		JsonObject->SetNumberField(Key, FCString::Atod(*Value));
	}
	else if (CastField<FBoolProperty>(Property))
	{
		JsonObject->SetBoolField(Key, FCString::ToBool(*Value));
	}
	else if (CastField<FStrProperty>(Property))
	{
		JsonObject->SetStringField(Key, Value);
	}
}

#undef TypeName_TArray
#undef TypeName_TSet
#undef TypeName_TMap