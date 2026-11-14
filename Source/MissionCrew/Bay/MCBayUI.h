// Small shared helpers for the bay UI / player code (agent F).
#pragma once

#include "CoreMinimal.h"
#include "Construction/MCConstructionTypes.h"

class AMCConstructionManager;
class UWorld;

namespace MCBayUI
{
	/** Bar colours: green / yellow / red. */
	MISSIONCREW_API FLinearColor CheckColor(EMCCheckState State);
	/** T/W colour with the 1.3 (red below) and 2.0 (green from) limits. TWR <= 0 is red. */
	MISSIONCREW_API FLinearColor TWRColor(float TWR);
	/** First AMCConstructionManager in the world (there is one per bay). */
	MISSIONCREW_API AMCConstructionManager* FindManager(const UWorld* World);
	/** Plain-English socket type name. */
	MISSIONCREW_API const TCHAR* SocketTypeName(EMCSocketType Type);
	/** The part's Description, or a line built from its numbers when the description is empty. */
	MISSIONCREW_API FString PartOneLiner(const UMCPartDef* Def);
	/** "35 kg | $6M | +300 W | 4.5 kN | fuel 600 kg" (only non-zero fields). */
	MISSIONCREW_API FString PartNumbers(const UMCPartDef* Def);
}
