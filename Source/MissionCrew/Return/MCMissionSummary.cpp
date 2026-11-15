#include "Return/MCMissionSummary.h"
#include "Core/MCMissionSubsystem.h"

FString UMCMissionSummary::StageName(int32 Stage)
{
	switch (Stage)
	{
	case 1: return TEXT("S1 Design & Assembly");
	case 2: return TEXT("S2 Launch & Transit");
	case 3: return TEXT("S3 Orbit & Landing");
	case 4: return TEXT("S4 Surface Operations");
	case 5: return TEXT("S5 Sample Preservation");
	case 6: return TEXT("S6 Return & Debrief");
	}
	return TEXT("?");
}

FString UMCMissionSummary::RoleLabel(int32 Role)
{
	switch (Role)
	{
	case 1: return TEXT("Systems Operator");
	case 2: return TEXT("Data Specialist");
	case 3: return TEXT("Pilot");
	case 4: return TEXT("Recovery Specialist");
	}
	return TEXT("Crew");
}

FString UMCMissionSummary::FormatTime(float Sec)
{
	const int32 S = FMath::Max(0, FMath::RoundToInt(Sec));
	return FString::Printf(TEXT("%d:%02d"), S / 60, S % 60);
}

FMCSummaryData UMCMissionSummary::Build(const UMCMissionSubsystem* Sub, int32 FallbackSeed)
{
	static const TCHAR* DefaultLesson[7] = {
		TEXT(""),
		TEXT("Balance mass, fuel and reliability before you lock the design."),
		TEXT("Cross-check the instruments: a stale display hides the real course."),
		TEXT("Keep a landing fuel reserve and call out hazards early."),
		TEXT("Plan the drill and rover work around power and the clock."),
		TEXT("A sample is only worth what its temperature log can prove."),
		TEXT("Queue the report before the blackout and fly the corridor.")
	};

	FMCSummaryData D;
	D.bValid = true;
	D.Seed = (Sub && Sub->MissionSeed != 0) ? Sub->MissionSeed : FallbackSeed;

	for (int32 St = 1; St <= 6; ++St)
	{
		FMCSummaryStage& S = D.Stages.AddDefaulted_GetRef();
		S.Stage = St;
		S.Name = StageName(St);
		S.Lesson = DefaultLesson[St];
		const FMCStageResult* R = Sub ? Sub->StageResults.FindByPredicate([St](const FMCStageResult& X) { return X.Stage == St; }) : nullptr;
		if (R)
		{
			S.bPlayed = true; S.Score = R->Score; S.DurationSec = R->DurationSec;
			if (!R->Lesson.IsEmpty()) S.Lesson = R->Lesson;
		}
		else if (St == 1 && Sub && Sub->bHasLoadout)
		{
			// S1 only stores per-role good/bad: derive a score (10 per good, -5 per bad).
			int32 G = 0, B = 0;
			for (int32 i : Sub->S1RoleGood) G += i;
			for (int32 i : Sub->S1RoleBad) B += i;
			S.bPlayed = true; S.Score = FMath::Max(0, G * 10 - B * 5);
		}
		D.TotalScore += S.Score;
		D.TotalSec += S.DurationSec;
	}

	// per-player accuracy by slot (role), name from the recorded roles
	for (int32 Role = 1; Role <= 4; ++Role)
	{
		FMCSummaryPlayer& P = D.Players.AddDefaulted_GetRef();
		P.Role = Role;
		P.Name = FString::Printf(TEXT("Slot %d"), Role);
		if (!Sub) continue;
		for (const FMCRoleRecord& R : Sub->Roles) if (R.Role == Role) { P.Name = R.PlayerName; break; }
		bool bS1Logged = false;
		for (const FMCAccuracyEntry& E : Sub->AccuracyLog)
		{
			if (E.Stage == 1) bS1Logged = true;
			if (E.Role != Role) continue;
			++P.Total;
			if (E.bCorrect) ++P.Good;
		}
		if (!bS1Logged && Sub->S1RoleGood.IsValidIndex(Role) && Sub->S1RoleBad.IsValidIndex(Role))
		{
			P.Good += Sub->S1RoleGood[Role];
			P.Total += Sub->S1RoleGood[Role] + Sub->S1RoleBad[Role];
		}
		if (P.Total > 0) P.AccuracyPct = 100.f * P.Good / P.Total;
	}
	return D;
}

FString UMCMissionSummary::ToText(const FMCSummaryData& D)
{
	FString S = FString::Printf(TEXT("MISSION SUMMARY  score %d  time %s  seed %d\n"), D.TotalScore, *FormatTime(D.TotalSec), D.Seed);
	for (const FMCSummaryStage& St : D.Stages)
		S += FString::Printf(TEXT("%s | %s | %s | %s\n"), *St.Name, St.bPlayed ? *FormatTime(St.DurationSec) : TEXT("--"),
			St.bPlayed ? *FString::FromInt(St.Score) : TEXT("--"), *St.Lesson);
	for (const FMCSummaryPlayer& P : D.Players)
		S += FString::Printf(TEXT("%s (%s): %s\n"), *P.Name, *RoleLabel(P.Role),
			P.AccuracyPct >= 0.f ? *FString::Printf(TEXT("%.0f%% (%d/%d)"), P.AccuracyPct, P.Good, P.Total) : TEXT("no data"));
	return S;
}
