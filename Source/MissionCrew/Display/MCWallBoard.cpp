#include "Display/MCWallBoard.h"
#include "Display/MCWallBoardWidget.h"
#include "Bay/MCResourceGraphsWidget.h"
#include "Components/WidgetComponent.h"

AMCWallBoard::AMCWallBoard()
{
	DrawSize = FIntPoint(1920, 1080);
	WorldWidthCm = 600.f;
}

TSubclassOf<UUserWidget> AMCWallBoard::GetDefaultWidgetClass() const
{
	return Mode == EMCWallBoardMode::Graphs ? TSubclassOf<UUserWidget>(UMCResourceGraphsWidget::StaticClass())
	                                        : TSubclassOf<UUserWidget>(UMCWallBoardWidget::StaticClass());
}

TSubclassOf<UUserWidget> AMCWallBoard::GetActiveWidgetClass() const
{
	if (WidgetClassOverride) return WidgetClassOverride;
	if (Mode == EMCWallBoardMode::Graphs && GraphsWidgetClass) return GraphsWidgetClass;
	if (Mode == EMCWallBoardMode::Checks && ChecksWidgetClass) return ChecksWidgetClass;
	return GetDefaultWidgetClass();
}

void AMCWallBoard::SetMode(EMCWallBoardMode NewMode)
{
	Mode = NewMode;
	ApplyDisplaySettings();
	if (WidgetComp && HasActorBegunPlay()) WidgetComp->InitWidget();
}
