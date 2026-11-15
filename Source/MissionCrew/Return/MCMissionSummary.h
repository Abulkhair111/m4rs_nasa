// Final mission summary data (M10). Built on the SERVER from UMCMissionSubsystem (stage results + accuracy log) and replicated by
// AMCReturnManager; missing stages get defaults. UMCMissionSummary is a pure helper (no state).
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MCMissionSummary.generated.h"

class UMCMissionSubsystem;

USTRUCT(BlueprintType)
struct FMCSummaryStage
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Stage = 0;
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) bool bPlayed = false;
	UPROPERTY(BlueprintReadOnly) int32 Score = 0;
	UPROPERTY(BlueprintReadOnly) float DurationSec = 0.f;
	UPROPERTY(BlueprintReadOnly) FString Lesson;
};

USTRUCT(BlueprintType)
struct FMCSummaryPlayer
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Role = 0;
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) int32 Good = 0;
	UPROPERTY(BlueprintReadOnly) int32 Total = 0;
	/** 0..100, -1 = no data. */
	UPROPERTY(BlueprintReadOnly) float AccuracyPct = -1.f;
};

USTRUCT(BlueprintType)
struct FMCSummaryData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bValid = false;
	UPROPERTY(BlueprintReadOnly) int32 Seed = 0;
	UPROPERTY(BlueprintReadOnly) int32 TotalScore = 0;
	UPROPERTY(BlueprintReadOnly) float TotalSec = 0.f;
	UPROPERTY(BlueprintReadOnly) TArray<FMCSummaryStage> Stages;   // always 6
	UPROPERTY(BlueprintReadOnly) TArray<FMCSummaryPlayer> Players; // roles 1..4
};

UCLASS()
class MISSIONCREW_API UMCMissionSummary : public UObject
{
	GENERATED_BODY()
public:
	/** Submission names 1..6. */
	UFUNCTION(BlueprintPure, Category = "Summary") static FString StageName(int32 Stage);
	/** Role names of S6 (the summary lists the player under the slot number). */
	UFUNCTION(BlueprintPure, Category = "Summary") static FString RoleLabel(int32 Role);
	/** Reads the mission log (Sub may be null = all defaults). FallbackSeed is used when the log has no seed. */
	static FMCSummaryData Build(const UMCMissionSubsystem* Sub, int32 FallbackSeed = 0);
	/** Multi-line plain text of the summary (log / tests). */
	static FString ToText(const FMCSummaryData& D);
	static FString FormatTime(float Sec);
};
