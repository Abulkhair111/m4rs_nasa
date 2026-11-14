// Wall board actor (agent I2): world-space widget, 1920x1080 px, ~600 x 340 cm, transparent background.
// Mode Checks = UMCWallBoardWidget (10 colour-coded checks + header + accuracy + message).
// Mode Graphs = UMCResourceGraphsWidget (bars/gauge). Faces actor +X. See MCWidgetDisplayActor.h.
// Place at: (-1315,900,180) yaw 0 (west wall, faces +X into the hall), (1315,-850,180) yaw 180 (east wall),
// graphs board (-1310,-900,205) yaw 0 with Mode = Graphs.
#pragma once

#include "CoreMinimal.h"
#include "Display/MCWidgetDisplayActor.h"
#include "MCWallBoard.generated.h"

UENUM(BlueprintType)
enum class EMCWallBoardMode : uint8
{
	Checks UMETA(DisplayName = "Preflight checks"),
	Graphs UMETA(DisplayName = "Resource graphs")
};

UCLASS(Blueprintable)
class MISSIONCREW_API AMCWallBoard : public AMCWidgetDisplayActor
{
	GENERATED_BODY()
public:
	AMCWallBoard();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board") EMCWallBoardMode Mode = EMCWallBoardMode::Checks;
	/** Optional WBP children (reparented to the C++ widgets). Null = code layout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board") TSubclassOf<UUserWidget> ChecksWidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board") TSubclassOf<UUserWidget> GraphsWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Board") void SetMode(EMCWallBoardMode NewMode);

protected:
	virtual TSubclassOf<UUserWidget> GetDefaultWidgetClass() const override;
	virtual TSubclassOf<UUserWidget> GetActiveWidgetClass() const override;
};
