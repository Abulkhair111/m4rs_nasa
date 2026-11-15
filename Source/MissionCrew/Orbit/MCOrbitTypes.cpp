#include "Orbit/MCOrbitTypes.h"

UMCOrbitConfig::UMCOrbitConfig()
{
	auto Add = [this](const TCHAR* Name, float Ice, float X, float Y, int32 Haz) {
		FMCOrbitSiteDef D; D.Name = Name; D.IceTrue = Ice; D.MapX = X; D.MapY = Y; D.RealHazards = Haz; Sites.Add(D);
	};
	// Placeholder sites (not real lunar locations). Richest ice is also the most hazardous: the trade-off the crew must find.
	Add(TEXT("SITE ALPHA"), 0.85f, -300.f, 420.f, 5);
	Add(TEXT("SITE BRAVO"), 0.70f, 150.f, 300.f, 2);
	Add(TEXT("SITE CHARLIE"), 0.55f, 420.f, -80.f, 1);
	Add(TEXT("SITE DELTA"), 0.30f, -120.f, -350.f, 0);
}
