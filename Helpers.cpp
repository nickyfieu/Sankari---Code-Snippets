#include "Helpers.h"

void LogText(ELogVerbosity::Type logLevel, const FString& text)
{
#ifdef DEBUG_UE_LOG

	switch (logLevel)
	{
	case ELogVerbosity::Display:
		UE_LOG(LogTemp, Display, TEXT("%s"), *text);
		break;
	case ELogVerbosity::Error:
		UE_LOG(LogTemp, Error, TEXT("%s"), *text);
		break;
	case ELogVerbosity::Fatal:
		UE_LOG(LogTemp, Fatal, TEXT("%s"), *text);
		break;
	case ELogVerbosity::Log:
		UE_LOG(LogTemp, Log, TEXT("%s"), *text);
		break;
	case ELogVerbosity::Verbose:
		UE_LOG(LogTemp, Verbose, TEXT("%s"), *text);
		break;
	case ELogVerbosity::VeryVerbose:
		UE_LOG(LogTemp, VeryVerbose, TEXT("%s"), *text);
		break;
	case ELogVerbosity::Warning:
		UE_LOG(LogTemp, Warning, TEXT("%s"), *text);
		break;
	default:
		LogText(ELogVerbosity::Warning, "Logging at wrong log level!");
		check(0);
		break;
	}

#endif
}