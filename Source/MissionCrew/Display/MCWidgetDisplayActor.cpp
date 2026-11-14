#include "Display/MCWidgetDisplayActor.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"

AMCWidgetDisplayActor::AMCWidgetDisplayActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	WidgetComp->SetupAttachment(Root);
	WidgetComp->SetWidgetSpace(EWidgetSpace::World);
	WidgetComp->SetBlendMode(EWidgetBlendMode::Transparent);
	WidgetComp->SetTwoSided(false);
	WidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComp->SetTickWhenOffscreen(false);
	WidgetComp->SetPivot(FVector2D(0.5f, 0.5f));
	WidgetComp->SetDrawAtDesiredSize(false);
}

TSubclassOf<UUserWidget> AMCWidgetDisplayActor::GetActiveWidgetClass() const
{
	return WidgetClassOverride ? WidgetClassOverride : GetDefaultWidgetClass();
}

void AMCWidgetDisplayActor::ApplyDisplaySettings()
{
	if (!WidgetComp) return;
	const FIntPoint S(FMath::Max(DrawSize.X, 16), FMath::Max(DrawSize.Y, 16));
	WidgetComp->SetDrawSize(FVector2D(S.X, S.Y));
	// The widget plane is 1 uu per pixel; scale it to the requested physical width.
	WidgetComp->SetRelativeScale3D(FVector(1.f, 1.f, 1.f) * (WorldWidthCm / (float)S.X));
	WidgetComp->SetRelativeRotation(FRotator(0.f, FaceYawDegrees, 0.f));
	WidgetComp->SetRelativeLocation(FVector::ZeroVector);
	TSubclassOf<UUserWidget> Cls = GetActiveWidgetClass();
	if (Cls && WidgetComp->GetWidgetClass() != Cls)
	{
		WidgetComp->SetWidgetClass(Cls);
	}
}

void AMCWidgetDisplayActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDisplaySettings();
}

void AMCWidgetDisplayActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyDisplaySettings();
	if (WidgetComp && !WidgetComp->GetWidget()) WidgetComp->InitWidget();
}
