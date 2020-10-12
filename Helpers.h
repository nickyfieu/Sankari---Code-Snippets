#pragma once
#include "CoreMinimal.h"
#include "Logging/LogVerbosity.h"

void LogText(ELogVerbosity::Type logLevel, const FString& text);

template<typename T>
bool IsValidIndex(int index, const TArray<T>& arr)
{
	return index > -1 && index < arr.Num();
}