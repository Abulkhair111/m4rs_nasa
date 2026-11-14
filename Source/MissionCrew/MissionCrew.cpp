#include "MissionCrew.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogMissionCrew);

// Third argument is the (deprecated) game name string; ASCII on purpose.
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, MissionCrew, "MissionCrew");
