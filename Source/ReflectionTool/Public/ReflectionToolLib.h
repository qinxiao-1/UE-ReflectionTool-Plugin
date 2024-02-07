// Fill out your copyright notice in the Description page of Project Settings.
// Copyright 2024 QinXiao, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Logging/LogMacros.h"
#include "ReflectionToolLib.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(ReflectionTool, Log, All);

class FJsonObject;

USTRUCT(BlueprintType)
struct FPropertyParserStruct
{
	GENERATED_BODY()

	// 变量名称，当存储的数据是 TArray、TSet、TMap 中的元素时，会有问题 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool")
	FString Name;

	// 变量类型
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool")
	FString TypeName;

	// 变量值，当存储的数据是 TArray、TSet、TMap 等复杂数据结构体时，该值为空，实际值都在 Children 中
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool")
	FString Value;
	
	// 是否有 Child
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool")
	bool bHaveChild = false;

	// 存储 TArray 中元素、Struct 中成员等
	TArray<FPropertyParserStruct> Children;
};

USTRUCT(BlueprintType)
struct FFuncParameter
{
	GENERATED_BODY()

	FFuncParameter()
	{
	}

	FFuncParameter(const FString& Type, const FString& Name)
		: Type(Type),
		  Name(Name)
	{
	}

	// 变量类型
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool|FuncParameter")
	FString Type = "";
	// 变量名称
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool|FuncParameter")
	FString Name = "";
};

USTRUCT(BlueprintType)
struct FFunctionInfo
{
	GENERATED_BODY()

	// 函数名称
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool|FunctionInfo")
	FString FunctionName = "";
	// 函数描述
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool|FunctionInfo")
	FString FunctionDesc = "";
	// 输入参数
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool|FunctionInfo")
	TArray<FFuncParameter> InParams = TArray<FFuncParameter>();
	// 输出参数
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReflectionTool|FunctionInfo")
	TArray<FFuncParameter> OutParams = TArray<FFuncParameter>();
};

UCLASS()
class REFLECTIONTOOL_API UReflectionToolLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

#pragma region 解析结构体

	// 转换任意结构体为 PPS(FPropertyParserStruct) 结构体
	template<typename InStructType>
	static void UStructToPropertyStruct(const InStructType& InStruct, FPropertyParserStruct& OutPropertyParserStruct);

	// 转换任意结构体为 TMap<FString, FString>
	template<typename InStructType>
	static void UStructToMap(const InStructType& InStruct, TMap<FString, FString>& ResultMap);
	
	// ↑ 中调用，首个结构体拿不到 FProperty，特殊处理
	static void GetStructProperty(const UStruct* StructClass, const void* Struct,  FPropertyParserStruct& OutPropertyParserStruct);

	// Struct to PPS
	static void StructToPropertyStruct(FStructProperty* StructProperty, const void* Addr, FPropertyParserStruct& OutPropertyParserStruct);

	// TArray to PPS
	static void TArrayToPropertyStruct(FArrayProperty* ArrayProperty, const void* Addr, FPropertyParserStruct& OutPropertyParserStruct);

	// TSet to PPS
	static void TSetToPropertyStruct(FSetProperty* SetProperty, const void* Addr, FPropertyParserStruct& OutPropertyParserStruct);

	// TMap to PPS
	static void TMapToPropertyStruct(FMapProperty* MapProperty, const void* Addr, FPropertyParserStruct& OutPropertyParserStruct);

	// Any Property to PPS
	static void PropertyToPropertyStruct(FProperty* Property, const void* Addr, FPropertyParserStruct& OutPropertyParserStruct);
	
#pragma endregion

#pragma region 构造结构体

	// 入口，顺便判断输入的 InPropertyParserStruct 是否正常
	template<typename OutStructType>
	static void ParsePPSToStruct(FPropertyParserStruct InPropertyParserStruct, OutStructType& OutStruct);

	// ↑ 中调用，首个结构体拿不到 FProperty，特殊处理
	static void ParserPropertyParserStruct(const UStruct* StructClass, void* Struct,  FPropertyParserStruct& InPropertyParserStruct);

	// PPS to TArray
	static void ParserPPSToArrayProperty(FArrayProperty* ArrayProperty, void* Addr, FPropertyParserStruct& InPropertyParserStruct);

	// PPS to TSet
	static void ParserPPSToSetProperty(FSetProperty* SetProperty, void* Addr, FPropertyParserStruct& InPropertyParserStruct);

	// PPS to TMap
	static void ParserPPSToMapProperty(FMapProperty* MapProperty, void* Addr, FPropertyParserStruct& InPropertyParserStruct);

	// PPS to Struct
	static void ParserPPSToStructProperty(FStructProperty* StructProperty, void* Addr, FPropertyParserStruct& InPropertyParserStruct);

	// Set Enum by FString（可能有更好的处理办法
	static void SetFStringToEnumProperty(FEnumProperty* EnumProperty, void* Addr, const FString& EnumString);

	// 解析任意 PPS 数据
	static void ParserPPSToProperty(FProperty* Property, void* Addr, FPropertyParserStruct& OutPropertyParserStruct);
	
	// 简单数据解析
	static void ParserSinglePPSToProperty(FProperty* Property, void* Addr, FPropertyParserStruct& InPropertyParserStruct);

	// 复杂数据解析
	static void ParserComplexPPSToProperty(FProperty* Property, void* Addr, FPropertyParserStruct& InPropertyParserStruct);
#pragma endregion
	
#if WITH_EDITOR
	/**
	 * @brief 获取指定类某个分组下的所有函数信息数据
	 * @param Class 
	 * @param Categories 指定分组
	 * @param Results 所有函数信息[ Contains: Name, InParams, OutParams ]
	 * @param ShowLog 是否打印LOG
	 * @return 是否成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ReflectionToolLib|Function")
	static bool GetFunctionsByCategories(UClass* Class, const TArray<FString>& Categories, TArray<FFunctionInfo>& Results,
	bool ShowLog = false);
	/**
	 * 获取指定类指定后缀的所有函数信息数据
	 * @param Class 
	 * @param Suffix 指定后缀
	 * @param Results 所有函数信息[ Contains: Name, InParams, OutParams ]
	 * @param ShowLog 是否打印LOG
	 * @return 是否成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ReflectionToolLib|Function")
	static bool GetFunctionsBySuffix(UClass* Class, const TArray<FString>& Suffix, TArray<FFunctionInfo>& Results,
										 bool ShowLog = false);
	
	UFUNCTION(BlueprintCallable, Category = "ReflectionToolLib|Function")
	static bool GetProperitesByCategories(UClass* Class, const TArray<FString>& Categories,
	                                      TArray<FFuncParameter>& Results, bool ShowLog = false);
#endif

#pragma region 执行函数

	// ↓中调用
	template<typename ... TReturns, typename ... TArgs>
	static bool InvokeFunction(UClass* Class, UObject* TargetObject, UFunction* Function,
	                           TTuple<TReturns...>& OutParams, TArgs&&... Args);
	// 执行函数，只能在 C++ 中使用
	template<typename ... TReturns, typename ... TArgs>
	static bool InvokeFunctionByName(UObject* TargetObject, const FName& FunctionName,
							   TTuple<TReturns...>& OutParams, TArgs&&... Args);
	/**
	 * @brief 通过函数名调用函数，目前传入的函数参数只支持几种基础类型
	 * @param TargetObject 目标对象
	 * @param FunctionName 函数名称
	 * @param InParams 目标函数的输入参数
	 * @param OutParams 目标函数的输出参数
	 * @return 是否调用成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ReflectionTool")
	static bool InvokeFunctionByName_Map(UObject* TargetObject, const FName& FunctionName,
	                                 const TMap<FString, FString>& InParams,
	                                 UPARAM(ref) TMap<FString, FString>& OutParams);
protected:
	// ↑中调用
	static bool InvokeFunctionByName(UClass* Class, UObject* TargetObject, UFunction* Function,
	                                 const TMap<FString, FString>& InParams, TMap<FString, FString>& OutParams);
public:
	/**
	 * @brief 通过函数名调用函数，目前传入的函数参数只支持几种基础类型
	 * @param TargetObject 
	 * @param FunctionName 
	 * @param InParams 参数列表
	 * @return 是否调用成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ReflectionTool")
	static bool InvokeFunctionByName_Array(UObject* TargetObject, const FName& FunctionName, const TArray<FString>& InParams);

protected:
	// ↑中调用
	static bool InvokeFunctionByName(UClass* Class, UObject* TargetObject, UFunction* Function, const TArray<FString>& InParams);
public:

#pragma endregion

#pragma region 使用Map设置结构体的值
	// 使用 Map 中的数据填充结构体
	template<typename InStructType>
	static void SetStructByMap(InStructType& InStruct, const TMap<FString, FString>& InMap);
	
	// 使用 Map 中的数据填充结构体 (辅助函数)
	static void SetStructValueByMap(const UStruct* StructClass, void* Struct, const TMap<FString, FString>& InMap);

	// 使用 Map 中的数据填充结构体 (辅助函数)
	static void SetStructValueByMap(FStructProperty* StructProperty, void* Addr, const TMap<FString, FString>& InMap);

#pragma endregion

#pragma region Blueprint Function
	/**
	 * @brief 获取解析结构体中的所有子节点
	 * @param PPS 目标解析结构体
	 * @return 所有子节点
	 */
	UFUNCTION(BlueprintPure, Category = "ReflectionTool")
	static TArray<FPropertyParserStruct> GetPPSChildren(const FPropertyParserStruct& PPS);
	/**
	 * @brief 给指定 PPS 添加 Child
	 * @param TargetPPS 
	 * @param ChildPPs 
	 */
	UFUNCTION(BlueprintCallable, Category = "ReflectionTool")
	static void AddPPSChild(UPARAM(ref) FPropertyParserStruct& TargetPPS, const FPropertyParserStruct& ChildPPs);
	/**
	 * @brief 将解析结构体解析成 TMap (无法处理存放复杂数据的 TArray TSet TMap，只处理简单数据的结果也一般)
	 * @param PPS 解析结构体
	 * @param ResultMap 结果
	 */
	UFUNCTION(BlueprintPure, Category = "ReflectionTool")
	static void GetKeyValueFromPPS(const FPropertyParserStruct& PPS, TMap<FString, FString>& ResultMap);

	/**
	 * @brief 蓝图泛型节点，获取结构体的解析结构体
	 * @param StructReference 被解析的结构体
	 * @param OutPropertyParserStruct 解析后的结构体
	 */
	UFUNCTION(BlueprintPure, CustomThunk, Category = "ReflectionTool", meta = (CustomStructureParam = "StructReference"))
	static void GetPropertyParserStruct(const int32& StructReference, FPropertyParserStruct& OutPropertyParserStruct);
	DECLARE_FUNCTION(execGetPropertyParserStruct)
	{
		// ----------------------------- Begin Get Property ----------------------------
		// 获取 Struct 数据
		Stack.MostRecentProperty = nullptr;
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.Step(Stack.Object, NULL);
		FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
		void* StructAddr = Stack.MostRecentPropertyAddress;

		if (!StructProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		P_GET_STRUCT_REF(FPropertyParserStruct, OutPropertyParserStruct);
		P_FINISH;
		// ----------------------------- End Get Property -----------------------------
		
		// 调用函数
		P_NATIVE_BEGIN;
		FGetPropertyParserStruct(StructAddr, StructProperty->Struct, OutPropertyParserStruct);
		P_NATIVE_END;
	}
	static void FGetPropertyParserStruct(const void* StructAddr, const UStruct* StructProperty, FPropertyParserStruct& OutPropertyParserStruct);
	
	/**
	 * @brief 使用 PPS 设置 Struct 的值
	 * @param StructReference 
	 * @param InPropertyParserStruct 
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "ReflectionTool", meta = (CustomStructureParam = "StructReference"))
	static void SetStructByPPS(const int32& StructReference, const FPropertyParserStruct& InPropertyParserStruct);
	DECLARE_FUNCTION(execSetStructByPPS)
	{
		// ----------------------------- Begin Get Property ----------------------------
		// 获取 Struct 数据
		Stack.MostRecentProperty = nullptr;
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.Step(Stack.Object, NULL);
		FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
		void* StructAddr = Stack.MostRecentPropertyAddress;

		if (!StructProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		P_GET_STRUCT_REF(FPropertyParserStruct, InPropertyParserStruct);
		P_FINISH;
		// ----------------------------- End Get Property -----------------------------
		
		// 调用函数
		P_NATIVE_BEGIN;
		FSetStructByPPS(StructAddr, StructProperty->Struct, InPropertyParserStruct);
		P_NATIVE_END;
	}
	static void FSetStructByPPS(void* StructAddr, const UStruct* StructProperty, FPropertyParserStruct& InPropertyParserStruct);
	
	/**
	 * @brief 蓝图泛型节点，将结构体转换为 TMap (无法处理存放复杂数据的 TArray TSet TMap，只处理简单数据的结果也一般)
	 * @param StructReference 被解析的结构体
	 * @param ResMap 解析后的 TMap<FString, FString>
	 */
	UFUNCTION(BlueprintPure, CustomThunk, Category = "ReflectionTool", meta = (CustomStructureParam = "StructReference"))
	static void GetStructPropertyMap(const int32& StructReference, TMap<FString, FString>& ResMap);
	DECLARE_FUNCTION(execGetStructPropertyMap)
	{
		// ----------------------------- Begin Get Property ----------------------------
		// 获取 Struct 数据
		Stack.MostRecentProperty = nullptr;
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.Step(Stack.Object, NULL);
		FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
		void* StructAddr = Stack.MostRecentPropertyAddress;

		if (!StructProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		// 获取 Map 数据
		Stack.MostRecentProperty = nullptr;
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.Step(Stack.Object, NULL);
		FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
		void* MapAddr = Stack.MostRecentPropertyAddress;
		if (!MapProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		P_FINISH;
		// ----------------------------- End Get Property -----------------------------
		
		// 调用函数
		P_NATIVE_BEGIN;
		FGetStructPropertyMap(StructAddr, StructProperty->Struct, MapAddr, MapProperty);
		P_NATIVE_END;
	}
	static void FGetStructPropertyMap(const void* StructAddr, const UStruct* StructProperty, void* MapAddr, FMapProperty* MapProperty);

	/**
	 * @brief 蓝图泛型节点，将 TMap 中的数据填入结构体 (无法处理存放复杂数据的 TArray TSet TMap，目前没考虑结构体套结构体的问题)
	 * @param StructReference 
	 * @param InMap 
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "ReflectionTool", meta = (CustomStructureParam = "StructReference"))
	static void SetStructPropertyByMap(const int32& StructReference, const TMap<FString, FString>& InMap);
	DECLARE_FUNCTION(execSetStructPropertyByMap)
	{
		// ----------------------------- Begin Get Property ----------------------------
		// 获取 Struct 数据
		Stack.MostRecentProperty = nullptr;
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.Step(Stack.Object, NULL);
		FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
		void* StructAddr = Stack.MostRecentPropertyAddress;

		if (!StructProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}

		// 获取 Map 数据
		Stack.MostRecentProperty = nullptr;
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.Step(Stack.Object, NULL);
		FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
		void* MapAddr = Stack.MostRecentPropertyAddress;
		if (!MapProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		P_FINISH;
		// ----------------------------- End Get Property -----------------------------
		
		// 调用函数
		P_NATIVE_BEGIN;
		FSetStructPropertyByMap(StructAddr, StructProperty->Struct, MapAddr, MapProperty);
		P_NATIVE_END;
	}
	static void FSetStructPropertyByMap(void* StructAddr, UStruct* StructProperty, const void* MapAddr, const FMapProperty* MapProperty);

	/**
	 * @brief 设置 PPS 的子节点
	 * @param PPS 
	 * @param PPSChildren 
	 */
	UFUNCTION(BlueprintCallable, Category = "ReflectionTool")
	static void SetPPSChildren(UPARAM(ref) FPropertyParserStruct& PPS, const TArray<FPropertyParserStruct>& PPSChildren);
#pragma endregion

#pragma region Helper Function

	static void SetJsonFieldByProperty(TSharedPtr<FJsonObject> JsonObject, FProperty* Property, const FString& Key, const FString& Value);
#pragma endregion 
};

template <typename InStructType>
void UReflectionToolLib::UStructToPropertyStruct(const InStructType& InStruct,
	FPropertyParserStruct& OutPropertyParserStruct)
{
	GetStructProperty(InStructType::StaticStruct(), &InStruct, OutPropertyParserStruct);
}

template <typename InStructType>
void UReflectionToolLib::UStructToMap(const InStructType& InStruct, TMap<FString, FString>& ResultMap)
{
	FPropertyParserStruct OutPropertyParserStruct;
	GetStructProperty(InStructType::StaticStruct(), &InStruct, OutPropertyParserStruct);
	GetKeyValueFromPPS(OutPropertyParserStruct, ResultMap);
}

template <typename OutStructType>
void UReflectionToolLib::ParsePPSToStruct(FPropertyParserStruct InPropertyParserStruct,
	OutStructType& OutStruct)
{
	if (InPropertyParserStruct.TypeName.IsEmpty())
	{
		if (InPropertyParserStruct.bHaveChild)
		{
			ParserPropertyParserStruct(OutStructType::StaticStruct(), &OutStruct, InPropertyParserStruct.Children[0]);
		}
		return;
	}
	ParserPropertyParserStruct(OutStructType::StaticStruct(), &OutStruct, InPropertyParserStruct);
}

template <typename ... TReturns, typename ... TArgs>
bool UReflectionToolLib::InvokeFunction(UClass* Class, UObject* TargetObject, UFunction* Function, TTuple<TReturns...>& OutParams, TArgs&&... Args)
{
	// 调用静态函数不需要对象
	Class = TargetObject == nullptr ? Class : TargetObject->GetClass();
	UObject* Context = TargetObject == nullptr ? Class : TargetObject;
	uint8* OutParamBuffer = (uint8*)&OutParams;

	if (!Function)
		return false;

	if (Function->HasAnyFunctionFlags(FUNC_Native))	// 看是否是 c++ function
	{
		// Forward : 左值引用转右值引用，等同于 std::forward
		// Forward使用原因：Args作为参数传入后，会变为左值，丢失右值属性，
		// Forward的目的：保证传入后还是右值
		TTuple<TArgs..., TReturns...> Params(Forward<TArgs>(Args)..., TReturns()...);
		// TTuple<TArgs..., TReturns...> Params((Args)..., TReturns()...);
		Context->ProcessEvent(Function, &Params);
		for (FProperty* Property = Function->PropertyLink; Property; Property = Property->PropertyLinkNext)
		{
			if (Property->PropertyFlags & CPF_OutParm)
			{
				void* PropBuffer = Property->ContainerPtrToValuePtr<void*>(&Params);
				// 复制，把内存中 n 个字节到目标内存区域
				Property->CopyCompleteValue(OutParamBuffer, PropBuffer);
				// 位移
				OutParamBuffer += Property->GetSize();
			}
		}
		return true;
	}

	TTuple<TArgs...> InParams(Forward<TArgs>(Args)...);
	void* FuncParamsBuffer = (uint8*)FMemory_Alloca(Function->ParmsSize);
	uint8* InParamsBuffer = (uint8*)&InParams;

	// 初始化
	for (FProperty* Property = Function->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (Property->GetName().StartsWith("__"))	// ignore __params
		{
			continue;
		}

		void* PropBuffer = Property->ContainerPtrToValuePtr<void*>(FuncParamsBuffer);
		if (Property->PropertyFlags & CPF_OutParm)
		{
			Property->CopyCompleteValue(PropBuffer, OutParamBuffer);
			OutParamBuffer += Property->GetSize();
		}
		else if (Property->PropertyFlags & CPF_Parm)
		{
			Property->CopyCompleteValue(PropBuffer, InParamsBuffer);
			InParamsBuffer += Property->GetSize();
		}
	}

	Context->ProcessEvent(Function, FuncParamsBuffer);
	OutParamBuffer = (uint8*)&OutParams;

	for (FProperty* Property = Function->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		if (Property->PropertyFlags & CPF_OutParm)
		{
			void* PropBuffer = Property->ContainerPtrToValuePtr<void*>(Property);
			Property->CopyCompleteValue(OutParamBuffer, PropBuffer);
			OutParamBuffer += Property->GetSize();
		}
	}

	return true;
}

template <typename ... TReturns, typename ... TArgs>
bool UReflectionToolLib::InvokeFunctionByName(UObject* TargetObject, const FName& FunctionName,
	TTuple<TReturns...>& OutParams, TArgs&&... Args)
{
	if (!TargetObject)
		return false;
	UFunction* Func = TargetObject->GetClass()->FindFunctionByName(FunctionName);
	if (!Func)
		return false;
	return InvokeFunction<TReturns...>(nullptr, TargetObject, Func, OutParams, Forward<TArgs>(Args)...);
}

template <typename InStructType>
void UReflectionToolLib::SetStructByMap(InStructType& InStruct, const TMap<FString, FString>& InMap)
{
	SetStructValueByMap(InStructType::StaticStruct(), &InStruct, InMap);
}
