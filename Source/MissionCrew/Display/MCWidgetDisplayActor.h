// World-space display base (agent I2): an actor with a UWidgetComponent. Used by AMCWallBoard, AMCMassScale, AMCPowerStrip.
//
// The widget plane FACES +X of the actor by default (a viewer standing at +X looking toward -X reads it correctly, text not
// mirrored). Rotate the actor, or set FaceYawDegrees (extra yaw of the widget plane only), to point it elsewhere. If a
// board ever appears mirrored in your placement, add 180 to FaceYawDegrees.
// DrawSize is the widget resolution in pixels; WorldWidthCm is the physical width, so the height follows the aspect ratio.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCWidgetDisplayActor.generated.h"

class UWidgetComponent;
class UUserWidget;
class USceneComponent;

UCLASS(Abstract, Blueprintable)
class MISSIONCREW_API AMCWidgetDisplayActor : public AActor
{
	GENERATED_BODY()
public:
	AMCWidgetDisplayActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display") TObjectPtr<UWidgetComponent> WidgetComp;

	/** Widget resolution in pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") FIntPoint DrawSize = FIntPoint(1920, 1080);
	/** Physical width of the display in cm (height = width * DrawSize.Y / DrawSize.X). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float WorldWidthCm = 600.f;
	/** Extra yaw of the widget plane relative to the actor (0 = faces actor +X). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float FaceYawDegrees = 0.f;
	/** Widget class shown. Null = the class's built-in C++ widget (code layout). Set a WBP child to restyle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") TSubclassOf<UUserWidget> WidgetClassOverride;

	/** Re-applies size / yaw / widget class. Called from OnConstruction and BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Display") virtual void ApplyDisplaySettings();

protected:
	/** The built-in widget class of this display. */
	virtual TSubclassOf<UUserWidget> GetDefaultWidgetClass() const { return nullptr; }
	virtual TSubclassOf<UUserWidget> GetActiveWidgetClass() const;
};
