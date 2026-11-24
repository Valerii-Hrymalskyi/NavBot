#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/blackmesa/bmbot.h>
#include <sdkports/sdk_takedamageinfo.h>
#include "scenario/bmbot_scenario_deathmatch_task.h"
#include "bmbot_deploy_tripmines.h"
#include "bmbot_find_health_task.h"
#include <bot/tasks_shared/bot_shared_prereq_tasks.h>
#include <bot/tasks_shared/bot_shared_plugin_command_tasks.h>
#include <bot/tasks_shared/bot_shared_default_combat_tasks.h>
#include "bmbot_tactical_task.h"

AITask<CBlackMesaBot>* CBlackMesaBotTacticalTask::InitialNextTask(CBlackMesaBot* bot)
{
	// BM is DM/TDM only
	return new CBlackMesaBotScenarioDeathmatchTask;
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	m_healthScanTimer.StartRandom(1.0f, 2.0f);
	m_deployTripminesTimer.StartRandom(5.0f, 15.0f);

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		if (m_healthScanTimer.IsElapsed())
		{
			m_healthScanTimer.StartRandom(1.0f, 2.0f);

			CBaseEntity* item;
			if (CBlackMesaBotFindHealthTask::IsPossible(bot, &item))
			{
				return PauseFor(new CBlackMesaBotFindHealthTask(item), "Searching for health!");
			}
		}
	}

	if (m_deployTripminesTimer.IsElapsed())
	{
		m_deployTripminesTimer.StartRandom(5.0f, 15.0f);

		Vector pos;
		if (CBlackMesaBotDeployTripminesTask::IsPossible(bot, pos))
		{
			return PauseFor(new CBlackMesaBotDeployTripminesTask(pos), "Opportunistically deploying tripmines!");
		}
	}

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat && bot->GetBehaviorInterface()->ShouldSeekAndDestroy(bot, threat) != ANSWER_NO)
	{
		return PauseFor(new CBotSharedDefaultCombatBehaviorTask<CBlackMesaBot, CBlackMesaBotPathCost>, "Attacking visible threat!");
	}

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnTaskResume(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	m_healthScanTimer.StartRandom(1.0f, 2.0f);
	m_deployTripminesTimer.StartRandom(5.0f, 15.0f);

	return Continue();
}

QueryAnswerType CBlackMesaBotTacticalTask::ShouldSeekAndDestroy(CBaseBot* baseBot, const CKnownEntity* them)
{
	CBlackMesaBot* me = static_cast<CBlackMesaBot*>(baseBot);

	if (me->GetHealthPercentage() > 0.95 && me->GetArmorPercentage() >= 0.80)
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnInjured(CBlackMesaBot* bot, const CTakeDamageInfo& info)
{
	// Turn towards the player who hurt us: an attacker behind the bot is outside its field of view, so the bot
	// would keep running away from it (same reaction as the DoD:S bots). Self damage (falling, own rockets)
	// and damage from the world are ignored.
	if (bot->GetDifficultyProfile()->GetGameAwareness() >= 15)
	{
		CBaseEntity* attacker = info.GetAttacker();

		if (attacker != nullptr && attacker != bot->GetEntity() && UtilHelpers::IsPlayer(attacker))
		{
			Vector pos = UtilHelpers::getWorldSpaceCenter(attacker);
			bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_DANGER, 1.5f, "Looking at attacker!");

			CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(attacker);
			known->UpdatePosition();
		}
	}

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnNavAreaChanged(CBlackMesaBot* bot, CNavArea* oldArea, CNavArea* newArea)
{
	TaskEventResponseResult<CBlackMesaBot> result = botprereqtasks::ImplementPrereqCheck<CBlackMesaBot, CBlackMesaBotTacticalTask, CNavArea, CBlackMesaBotPathCost>(this, bot, newArea);

	if (result.IsRequestingChange())
	{
		return result;
	}

	return TryContinue();
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotTacticalTask::OnPluginCommand(CBlackMesaBot* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data)
{
	return plugincommandtask::ImplementPluginCommandTasks<CBlackMesaBotTacticalTask, CBlackMesaBot, CBlackMesaBotPathCost>(this, bot, type, data);
}
