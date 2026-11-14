// Role 3 Pilot flight console (agent S2b).
//
// Reparent a WBP to UMCPilotPanel and add widgets with EXACTLY these names (all optional):
//   ScopeCanvas   CanvasPanel  the scope area; ring and dot are canvas children (centre anchor, alignment 0.5/0.5)
//   CorridorRing  Image        positioned/sized by code: diameter = 2 * CorridorKm * PxPerKm
//   MissDot       Image        positioned by code at (DispX, -DispY) * PxPerKm (clamped to the scope edge)
//   BurnLeft / BurnRight / BurnUp / BurnDown  Button  actions 300..303 (enabled only while a window is open)
//   MissText      TextBlock    "Predicted miss 12.3 km (x +10.1, y -7.0) INSIDE/OUTSIDE"
//   WindowText    TextBlock    open window countdown or next window
//   FuelText      TextBlock    fuel left
// Scale: 1.6 px per km (spec). With no layout in the Blueprint a complete fallback layout is built
// (ring/dot are rounded-box brushes, so no texture is needed).
#pragma once

#include "CoreMinimal.h"
#include "Transit/UI/MCTransitPanelBase.h"
#include "MCPilotPanel.generated.h"

class UCanvasPanel;
class UImage;

UCLASS(Blueprintable)
class MISSIONCREW_API UMCPilotPanel : public UMCTransitPanelBase
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCanvasPanel> ScopeCanvas;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> CorridorRing;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> MissDot;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BurnLeft;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BurnRight;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BurnUp;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BurnDown;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> WindowText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FuelText;

	/** Scope scale, pixels per km. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float PxPerKm = 1.6f;
	/** Fallback scope size in pixels (square). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float ScopeSize = 260.f;

protected:
	virtual void BuildFallbackLayout() override;
	virtual void BindButtons() override;
	virtual void RefreshPanel() override;
};
