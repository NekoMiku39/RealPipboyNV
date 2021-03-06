#include "DataManager.h"

#include "../RealPipboy/Packets/packets.h"
#include "../RealPipboy/Communication/CommunicationManager.h"

#include <Windows.h>

#include <chrono>
#include <ctime>
#include <cmath>
#include <map>
#include <vector>

#include "nvse/PluginAPI.h"
#include "nvse/GameAPI.h"
#include "nvse/GameTypes.h"
#include "nvse/GameObjects.h"
#include "nvse/GameForms.h"
#include "nvse/GameSettings.h"
#include "nvse/GameData.h"
#include "nvse/InventoryInfo.h"
#include "nvse/GameEffects.h"
#include "nvse/GameTiles.h"
#include "nvse/Utilities.h"
#include "nvse/GameTasks.h"
#include "nvse/GameSettings.h"
#include "nvse/GameRTTI.h"
#include "nvse/GameScript.h"
#include "nvse/GameOSDepend.h"
#include "nvse/GameUI.h"
#include "nvse/GameExtraData.h"
#include "nvse/GameBSExtraData.h"
#include "nvse/GameProcess.h"
#include "nvse/NiTypes.h"
#include "nvse/NiNodes.h"

#include "RealPipboy/DataTypes/WeaponItem.h"
#include "RealPipboy/DataTypes/ApparelItem.h"
#include "RealPipboy/DataTypes/AidItem.h"
#include "RealPipboy/DataTypes/MiscItem.h"
#include "RealPipboy/DataTypes/AmmoItem.h"
#include "RealPipboy/DataTypes/GeneralItems.h"
#include "RealPipboy/DataTypes/Quest.h"

//todo effects, location name

// Edits TESCaravanMoney, TESObjectMisc, TESObjectBOOK, ActorValueInfo, PlayerCharacter, bgs voice type, extraRadioData,
// TESCaravanCard

// Member functions: BSFile, ModInfo

extern NVSEScriptInterface *g_script;

DataManager DataManager::m_ourInstance;

const char *SPECIAL_DESCRIPTIONS [] = {
	"Strength is a measure of your raw physical power. It affects how much you can carry, "
	"the power of all melee attacks, and your effectiveness with many heavy weapons.",

	"A high Perception grants a bonus to the Explosives, Lockpick and Energy Weapons "
	"skills, and determines when red compass markings appear (which indicate threats).",

	"Endurance is a measure of your overall physical fitness. A high Endurance gives "
	"bonuses to health, environmental resistances, and the Survival and Unarmed skills.",

	"Having a high Charisma will improve people's disposition toward you, and give "
	"bonuses to both the Barter and Speech skills.  ",

	"Intelligence affects the Science, Repair and Medicine skills. The higher your "
	"Intelligence, the more Skill Points you'll be able to distribute when you level up.",

	"Agility affects your Guns and Sneak skills, and the number of Action Points "
	"available for V.A.T.S.",

	"Raising your Luck will raise all of your skills a little. Having a high Luck "
	"will also improve your critical chance with all weapons.    "
};

const char *SKILL_DESCRIPTIONS [] = {
	"The Barter skill affects the prices you get for buying and selling items. "
	"In general, the higher your Barter skill, the lower your prices on purchased items.  ",

	"TODO big guns",

	"The Energy Weapons skill determines your effectiveness with any weapon that uses "
	"Small Energy Cells, Micro Fusion Cells, EC Packs, or Flamer Fuel as ammunition.",

	"The Explosives skill determines the ease of disarming any hostile mines and the "
	"effectiveness of any explosive weapon (all mines, all grenades, Missile Launcher, "
	"Fat Man, etc.).",

	"The Lockpick skill is used to open locked doors and containers.  ",

	"The Medicine skill determines how many Hit Points you'll replenish upon using a"
	" Stimpak, and the effectiveness of Rad-X and RadAway.",

	"The Melee Weapons skill determines your effectiveness with any melee weapon, "
	"from the simple lead pipe all the way up to the high-tech Super Sledge.",

	"The Repair skill allows you to maintain any weapons and apparel. In addition, "
	"Repair allows you to create items and Guns ammunition at reloading benches.",

	"The Science skill represents your combined scientific knowledge, and is "
	"primarily used to hack restricted computer terminals. It can also be used to"
	" recycle Energy Weapon ",

	"Guns determines your effectiveness with any weapon that uses conventional "
	"ammunition (.22 LR, .357 Magnum, 5mm, 10mm, 5.56mm, .308, .45-70 Gov't etc.).",

	"The higher your Sneak skill, the easier it is to remain undetected, steal "
	"an item, or pick someone's pocket. Successfully attacking while undetected "
	"grants an automatic critical hit.",

	"The Speech skill governs how much you can influence someone through dialogue, "
	"and gain access to information they might otherwise not want to share.   ",

	"The Survival skill increases the Hit Points you receive from food and drink. "
	"It also helps you create consumable items at campfires.",

	"The Unarmed skill is used for fighting without a weapon, or with weapons "
	"designed for hand-to-hand combat, like Brass Knuckles, Power Fists, and "
	"Displacer Gloves."
}; 

const char *SKILL_BADGES [] = {
		"",
		"",
		"textures/interface/icons/typeicons/weap_skill_icon_energy.dds",
		"textures/interface/icons/typeicons/weap_skill_icon_explosives.dds",
		"",
		"",
		"textures/interface/icons/typeicons/weap_skill_icon_melee.dds",
		"",
		"",
		"textures/interface/icons/typeicons/weap_skill_icon_sm_arms.dds",
		"",
		"",
		"",
		"textures/interface/icons/typeicons/weap_skill_icon_unarmed.dds"
	};

const int NUM_GENERAL_STATISTICS = 43;

bool (__cdecl *getPCMiscStat)(TESObjectREFR *ref, int id, int unk0, double *retVal) = 
(bool(__cdecl *)(TESObjectREFR *, int, int, double *))0x005A3310;

bool(__cdecl *isHardcore)(TESObjectREFR *ref, void *arg0, void *arg1, double *retVal) =
(bool(__cdecl *)(TESObjectREFR *, void *arg0, void *arg1, double *))0x005A5F90;

bool(__cdecl *isDisabled)(TESObjectREFR *ref, void *arg0, void *arg1, double *retVal) =
(bool(__cdecl *)(TESObjectREFR *, void *arg0, void *arg1, double *))0x0059CF50;

static char tempBuffer[1024];

DataManager::DataManager() :
	m_hardcore(false), m_numStimpacks(0), 
	m_numDrBag(0), m_numRadaway(0), m_numRadX(0)
{
	m_miscStatisticsValues.resize(NUM_GENERAL_STATISTICS, 0);
}

void DataManager::init()
{
	m_radioIds.load("RadioStations");
	m_dehydrationEffIds.load("DehydrationEffects");
	m_radiationEffIds.load("RadiationEffects");
	m_starvationEffIds.load("StarvationEffects");
	m_sleepEffIds.load("SleepDeprivationEffects");
}


std::string DataManager::getModifierExtra(float val)
{
	if (val > 0) {
		return "(+)";
	}
	else if (val < 0) {
		return "(-)";
	}
	else {
		return "";
	}
}

DataManager::~DataManager()
{
}

DataManager & DataManager::getInstance()
{
	return m_ourInstance;
}

void test(void *) {
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	InterfaceManager *ifmngr = InterfaceManager::GetSingleton();
	FOPipboyManager *pipboy = ifmngr->pipboyManager;
	DataHandler *dh = DataHandler::Get();

	IDebugLog::Indent();
	IDebugLog::Outdent();
}

void DataManager::registerUpdates(Scheduler &scheduler)
{
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateGameInfo, 5000));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updatePlayerInfo, 200));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateSpecial, 2000));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateSkills, 2000));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updatePerks, 2000));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateStats, 2000));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateEffects, 1500));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateWorldInfo, 500));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateInventory, 1000));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateNotes, 3000));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateQuests, 1200));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateMapMarkers, 5000));
	m_updateIds.push_back(scheduler.addPeriodic(DataManager::updateRadio, 1000));
	//m_updateIds.push_back(scheduler.addPeriodic(test, 5000));
}

void DataManager::deregisterUpdates(Scheduler &scheduler)
{
	for (std::vector<int>::iterator it = m_updateIds.begin(); it != m_updateIds.end(); ++it) {
		scheduler.removeTask(*it);
	}
}

void DataManager::update()
{
	
}

void DataManager::updateGameInfo(void *)
{
	DataManager &dm = DataManager::getInstance();
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	if (player == NULL)
		return;
	double isHardcoreResult = 0;
	if (isHardcore(player, NULL, NULL, &isHardcoreResult)) {
		dm.m_hardcore = isHardcoreResult != 0;
	}
	CommunicationManager::getInstance().sendPacket(new SetGameInfoPacket(0, dm.m_hardcore));
}

void DataManager::updatePlayerInfo(void *)
{
	DataManager &dm = DataManager::getInstance();
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	if (player == NULL) {
		return;
	}
	TESFullName *fullName = player->GetFullName();

	dm.m_level = player->avOwner.GetLevel();
	dm.m_hp = (int16_t)player->avOwner.GetActorValueI(eActorVal_Health);
	dm.m_maxHP = (int16_t) player->avOwner.GetPermanentActorValueI(eActorVal_Health);
	dm.m_ap = (int16_t) player->avOwner.GetActorValueI(eActorVal_ActionPoints);
	dm.m_maxAP = (int16_t) player->avOwner.GetPermanentActorValueI(eActorVal_ActionPoints);
	dm.m_xp = (int32_t) player->avOwner.GetActorValueI(eActorVal_XP);
	double iXPBase = 0;
	double iXPBumpBase = 0;
	GetNumericGameSetting("iXPBase", &iXPBase);
	GetNumericGameSetting("iXPBumpBase", &iXPBumpBase);
	dm.m_nextXP = dm.m_level * ((dm.m_level - 1)/2.0f * iXPBumpBase + iXPBase);
	dm.m_head = (int8_t)player->avOwner.GetActorValueI(eActorVal_Head);
	dm.m_torso = (int8_t) player->avOwner.GetActorValueI(eActorVal_Torso);
	dm.m_leftArm = (int8_t) player->avOwner.GetActorValueI(eActorVal_LeftArm);
	dm.m_rightArm = (int8_t) player->avOwner.GetActorValueI(eActorVal_RightArm);
	dm.m_leftLeg = (int8_t) player->avOwner.GetActorValueI(eActorVal_LeftLeg);
	dm.m_rightLeg = (int8_t) player->avOwner.GetActorValueI(eActorVal_RightLeg);

	dm.m_radresist = (int8_t) player->avOwner.GetActorValueI(eActorVal_RadResistance);
	dm.m_rads = (int16_t) player->avOwner.GetActorValueI(eActorVal_RadLevel);
	dm.m_dehydration = (int16_t) player->avOwner.GetActorValueI(eActorVal_Dehydration);
	dm.m_hunger = (int16_t) player->avOwner.GetActorValueI(eActorVal_Hunger);
	dm.m_sleep = (int16_t) player->avOwner.GetActorValueI(eActorVal_Sleepdeprevation);

	dm.m_weight = (int16_t) player->avOwner.GetActorValueI(eActorVal_InventoryWeight);
	dm.m_maxWeight = (int16_t) player->avOwner.GetActorValueI(eActorVal_CarryWeight);
	
	dm.m_dr = player->avOwner.GetActorValue(eActorVal_DamageResistance);
	dm.m_dt = player->avOwner.GetActorValue(eActorVal_Damagethreshold) -2; // For some reason it is offset with -2

	SInt32 caps = 0;
	TESForm *form = GetItemByRefID(player, 0xF, &caps); // 0xF is ID of Caps001	
	dm.m_caps = caps;

	dm.m_karma = (int16_t)player->avOwner.GetActorValueI(eActorVal_Karma);

	float x = player->posX;
	float y = player->posY;
	float z = player->posZ;
	float rotX = player->rotX;
	float rotY = player->rotY;
	float rotZ = player->rotZ;

	CommunicationManager::getInstance().sendPacket(new SetPlayerInfoPacket(dm.m_level,
		dm.m_hp, dm.m_maxHP, dm.m_ap, dm.m_maxAP, dm.m_xp, dm.m_nextXP, dm.m_head, 
		dm.m_torso, dm.m_leftArm, dm.m_rightArm, dm.m_leftLeg, dm.m_rightLeg, 
		dm.m_radresist, dm.m_rads, 1000, dm.m_dehydration, 1000, dm.m_hunger, 1000, 
		dm.m_sleep, 1000, dm.m_weight, dm.m_maxWeight, dm.m_dr, dm.m_dt, caps, 
		dm.m_karma, fullName->name.CStr(), x, y, z, rotX, rotY, rotZ));
}

void DataManager::updateSpecial(void *)
{
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	if (player == NULL)
		return;
	
	// AV info
	ActorValueInfo *strengthInfo = GetActorValueInfo(eActorVal_Strength);
	ActorValueInfo *perceptionInfo = GetActorValueInfo(eActorVal_Perception);
	ActorValueInfo *enduranceInfo = GetActorValueInfo(eActorVal_Endurance);
	ActorValueInfo *charismaInfo = GetActorValueInfo(eActorVal_Charisma);
	ActorValueInfo *intelligenceInfo = GetActorValueInfo(eActorVal_Intelligence);
	ActorValueInfo *agilityInfo = GetActorValueInfo(eActorVal_Agility);
	ActorValueInfo *luckInfo = GetActorValueInfo(eActorVal_Luck);

	// Get display level
	int strengthLevel = player->avOwner.GetActorValueI(eActorVal_Strength);
	int perceptionLevel = player->avOwner.GetActorValueI(eActorVal_Perception);
	int enduranceLevel = player->avOwner.GetActorValueI(eActorVal_Endurance);
	int charismaLevel = player->avOwner.GetActorValueI(eActorVal_Charisma);
	int intelligenceLevel = player->avOwner.GetActorValueI(eActorVal_Intelligence);
	int agilityLevel = player->avOwner.GetActorValueI(eActorVal_Agility);
	int luckLevel = player->avOwner.GetActorValueI(eActorVal_Luck);

	// Get modifiers
	float strengthModifier = player->avOwner.getAVModifier(eActorVal_Strength);
	float perceptionModifier = player->avOwner.getAVModifier(eActorVal_Perception);
	float enduranceModifier = player->avOwner.getAVModifier(eActorVal_Endurance);
	float charismaModifier = player->avOwner.getAVModifier(eActorVal_Charisma);
	float intelligenceModifier = player->avOwner.getAVModifier(eActorVal_Intelligence);
	float agilityModifier = player->avOwner.getAVModifier(eActorVal_Agility);
	float luckModifier = player->avOwner.getAVModifier(eActorVal_Luck);

	if ((strengthLevel >= 10 && strengthModifier > 0) ||
		(strengthLevel <= 0 && strengthModifier < 0)) {
		strengthModifier = 0;
	}
	if ((perceptionLevel >= 10 && perceptionModifier > 0) ||
		(perceptionLevel <= 0 && perceptionModifier < 0)) {
		perceptionModifier = 0;
	}
	if ((enduranceLevel >= 10 && enduranceModifier > 0) ||
		(enduranceLevel <= 0 && enduranceModifier < 0)) {
		enduranceModifier = 0;
	}
	if ((charismaLevel >= 10 && charismaModifier > 0) ||
		(charismaLevel <= 0 && charismaModifier < 0)) {
		charismaModifier = 0;
	}
	if ((intelligenceLevel >= 10 && intelligenceModifier > 0) ||
		(intelligenceLevel <= 0 && intelligenceModifier < 0)) {
		intelligenceModifier = 0;
	}
	if ((agilityLevel >= 10 && agilityModifier > 0) ||
		(agilityLevel <= 0 && agilityModifier < 0)) {
		agilityModifier = 0;
	}
	if ((luckLevel >= 10 && luckModifier > 0) ||
		(luckLevel <= 0 && luckModifier < 0)) {
		luckModifier = 0;
	}
	if (strengthLevel > 10) strengthLevel = 10;
	if (perceptionLevel > 10) perceptionLevel = 10;
	if (enduranceLevel > 10) enduranceLevel = 10;
	if (charismaLevel > 10) charismaLevel = 10;
	if (intelligenceLevel > 10) intelligenceLevel = 10;
	if (agilityLevel > 10) agilityLevel = 10;
	if (luckLevel > 10) luckLevel = 10;
	if (strengthLevel < 0) strengthLevel = 0;
	if (perceptionLevel < 0) perceptionLevel = 0;
	if (enduranceLevel < 0) enduranceLevel = 0;
	if (charismaLevel < 0) charismaLevel = 0;
	if (intelligenceLevel < 0) intelligenceLevel = 0;
	if (agilityLevel < 0) agilityLevel = 0;
	if (luckLevel < 0) luckLevel = 0;

	const std::string iconPrefix = "textures\\";

	DataManager &i = DataManager::getInstance();
	i.m_special.clear();
	i.m_special.push_back(StatisticsInfoItem(strengthInfo->name.CStr(),
		iconPrefix + strengthInfo->icon.ddsPath.CStr(), "", SPECIAL_DESCRIPTIONS[0],
		std::to_string(strengthLevel), getModifierExtra(strengthModifier)));
	i.m_special.push_back(StatisticsInfoItem(perceptionInfo->name.CStr(),
		iconPrefix + perceptionInfo->icon.ddsPath.CStr(), "", SPECIAL_DESCRIPTIONS[1],
		std::to_string(perceptionLevel), getModifierExtra(perceptionModifier)));
	i.m_special.push_back(StatisticsInfoItem(enduranceInfo->name.CStr(),
		iconPrefix + enduranceInfo->icon.ddsPath.CStr(), "", SPECIAL_DESCRIPTIONS[2],
		std::to_string(enduranceLevel), getModifierExtra(enduranceModifier)));
	i.m_special.push_back(StatisticsInfoItem(charismaInfo->name.CStr(),
		iconPrefix + charismaInfo->icon.ddsPath.CStr(), "", SPECIAL_DESCRIPTIONS[3],
		std::to_string(charismaLevel), getModifierExtra(charismaModifier)));
	i.m_special.push_back(StatisticsInfoItem(intelligenceInfo->name.CStr(),
		iconPrefix + intelligenceInfo->icon.ddsPath.CStr(), "", SPECIAL_DESCRIPTIONS[4],
		std::to_string(intelligenceLevel), getModifierExtra(intelligenceModifier)));
	i.m_special.push_back(StatisticsInfoItem(agilityInfo->name.CStr(),
		iconPrefix + agilityInfo->icon.ddsPath.CStr(), "", SPECIAL_DESCRIPTIONS[5],
		std::to_string(agilityLevel), getModifierExtra(agilityModifier)));
	i.m_special.push_back(StatisticsInfoItem(luckInfo->name.CStr(),
		iconPrefix + luckInfo->icon.ddsPath.CStr(), "", SPECIAL_DESCRIPTIONS[6],
		std::to_string(luckLevel), getModifierExtra(luckModifier)));

	CommunicationManager::getInstance().sendPacket(new SetStatsItemsPacket(0, i.m_special));
}

void DataManager::updateSkills(void *)
{
	const std::string iconPrefix = "textures\\";
	DataManager &i = DataManager::getInstance();
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	if (player == NULL)
		return;

	i.m_skills.clear();
	for (int av = eActorVal_SkillsStart; av <= eActorVal_SkillsEnd; av++) {
		if (true/* TODO: Fallout NV check */  && av == eActorVal_BigGuns) {
			continue;
		}

		ActorValueInfo *info = GetActorValueInfo(av);
		int value = player->avOwner.GetActorValueI(av);
		float modifier = player->avOwner.getAVModifier(av);
		int baseValue = player->avOwner.GetPermanentActorValueI(av);
		if (modifier == 0) {
			modifier = value - baseValue;
		}

		i.m_skills.push_back(StatisticsInfoItem(info->name.CStr(), 
			iconPrefix + info->icon.ddsPath.CStr(), SKILL_BADGES[av - eActorVal_SkillsStart],
			SKILL_DESCRIPTIONS[av - eActorVal_SkillsStart], 
			std::to_string(value), getModifierExtra(modifier)));
	}

	CommunicationManager::getInstance().sendPacket(new SetStatsItemsPacket(1, i.m_skills));
}

void DataManager::updatePerks(void *)
{
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	DataHandler *dataHandler = DataHandler::Get();
	DataManager &dm = DataManager::getInstance();
	const std::string iconPrefix = "textures\\";
	if (player == NULL || dataHandler == NULL)
		return;

	dm.m_perks.clear();

	tList<BGSPerk>::Iterator begin = dataHandler->perkList.Begin(); 
	for (tList<BGSPerk>::Iterator it = begin; !it.End(); ++it) {
		BGSPerk *perk = const_cast<BGSPerk *>(*it);
		UInt8 rank = player->GetPerkRank(perk, false);
		UInt8 altRank = player->GetPerkRank(perk, true);

		const char *desc = perk->description.Get(perk, 'CSED');

		if (rank > 0 && ((perk->unk38[1] & 1) == 0)) {
			std::string icon = "";
			if (perk->icon.ddsPath.m_dataLen > 0) {
				icon = iconPrefix + perk->icon.ddsPath.CStr();
			}
			dm.m_perks.push_back(StatisticsInfoItem(perk->fullName.name.CStr(), 
				icon, "", desc));
		}
	}

	CommunicationManager::getInstance().sendPacket(new SetStatsItemsPacket(2, dm.m_perks));
}

void DataManager::updateStats(void *)
{
	DataManager &dm = DataManager::getInstance();
	GameSettingCollection *gs = GameSettingCollection::GetSingleton();
	Setting *setting;
	char settingName[20];

	if (gs == NULL)
		return;

	dm.m_stats.clear();

	for (int i = 0; i < NUM_GENERAL_STATISTICS; i++) {
		sprintf(settingName, "sMiscStat%03d", i);
		if (!gs->GetGameSetting(settingName, &setting))
			continue;
		double value = 0;
		if (getPCMiscStat(NULL, i, 0, &value)) {
			dm.m_miscStatisticsValues[i] = (int)value;
			dm.m_stats.push_back(StatisticsInfoItem(setting->Get(), "", "", "",
				std::to_string((int)value), ""));
		}
	}
	
	CommunicationManager::getInstance().sendPacket(new SetStatsItemsPacket(3, dm.m_stats));
}

void updateStatusEffect(const std::set<UInt32> &refs, ActiveEffect  *effect, std::vector<StatusEffect> &dataOut) {
	std::string name = effect->item->name.CStr();
	for (std::set<UInt32>::iterator it = refs.cbegin(); it != refs.cend(); ++it) {
		SpellItem *spell = DYNAMIC_CAST(LookupFormByID(*it), TESForm, SpellItem);
		if (spell == NULL || spell->magicItem.name.m_dataLen <= 0)
			continue;
		if (name == spell->magicItem.name.CStr()) {
			dataOut.clear();
			
			for (tList<EffectItem>::Iterator itEff = effect->item->list.list.Begin(); !itEff.End(); ++itEff) {
				std::string abbr = "";
				std::string value = "";
				int avOrOther = itEff->actorValueOrOther;
				if (avOrOther > 0) {
					ActorValueInfo *avInfo = GetActorValueInfo(avOrOther);
					if (avInfo == NULL)
						continue;
					int magnitude = itEff->magnitude;
					if (itEff->setting->effectFlags & 0x4) {
						magnitude = -magnitude;
					}
					dataOut.push_back(StatusEffect(avInfo->abbrev.CStr(), magnitude));
				}
			}
		}
	}
}

std::string getEffectsString(EffectItemList *list) {
	std::string effects = "";
	//_MESSAGE("%30s: %d %08x %08x %08x %08x", name.c_str(), effect->spellType, effect->effectItem->setting->archtype, effect->unk44, flags1, flags2);
	for (tList<EffectItem>::Iterator itEff = list->list.Begin(); !itEff.End(); ++itEff) {
		int avOrOther = itEff->actorValueOrOther;
		if (avOrOther > 0) {
			ActorValueInfo *avInfo = GetActorValueInfo(avOrOther);
			if (avInfo == NULL)
				continue;
			int magnitude = itEff->magnitude;
			if (itEff->setting->effectFlags & 0x4) {
				magnitude = -magnitude;
			}
			sprintf_s(tempBuffer, "%s %+d, ", avInfo->abbrev.CStr(), magnitude);
			effects.append(tempBuffer);
		}
		else {
			if ((itEff->setting->effectFlags & 0x2000) != 0) {
				sprintf_s(tempBuffer, "%s, ", itEff->setting->fullName.name.CStr());
				effects.append(tempBuffer);
			}
		}
	}
	if (effects.length() >= 2) {
		// remove ", " from end
		effects = effects.substr(0, effects.length() - 2);
	}
	return effects;
}

void DataManager::updateEffects(void *)
{
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	DataManager &dm = DataManager::getInstance();

	if (player == NULL)
		return;
	dm.m_playerEffects.clear();

	EffectNode *effectList = player->magicTarget.GetEffectList();
	if (effectList == NULL)
		return;	
	//_MESSAGE("Effects %d:", effectList->Count());
	for (EffectNode::Iterator it = effectList->Begin(); !it.End(); ++it) {
		ActiveEffect *effect = it.Get();
		UInt32 flags1 = effect->unk18;
		UInt32 flags2 = effect->effectItem->setting->effectFlags;
		if(effect == NULL || effect->item == NULL || effect->flags12 == 1 || 
			(flags2 & 0x02) != 0x02 || (flags1 & 0x0e) != 0x00 ||
			(effect->spellType == 4 && (flags1 & 0x80) == 0) || (flags2 & 0x60) == 0)
			continue;
		MagicItem *item = effect->item;
		std::string name = item->name.CStr();
		//_MESSAGE("Effect %30s: %d %02x %02x %08x %08x %08x %08x %08x %08x %2.2f %2.2f %s", name.c_str(), effect->spellType, effect->flags10, effect->flags12, effect->unk14, effect->unk18, effect->unk30, effect->unk34, effect->unk38, effect->unk44, effect->magnitude, effect->duration, effects.c_str());
		std::string effects = getEffectsString(&item->list);
		dm.m_playerEffects.push_back(PlayerEffect(name, effects));

		updateStatusEffect(dm.m_radiationEffIds.getList(), effect, dm.m_radEffects);
		updateStatusEffect(dm.m_dehydrationEffIds.getList(), effect, dm.m_h2oEffects);
		updateStatusEffect(dm.m_starvationEffIds.getList(), effect, dm.m_fodEffects);
		updateStatusEffect(dm.m_sleepEffIds.getList(), effect, dm.m_slpEffects);
	}

	CommunicationManager::getInstance().sendPacket(new SetPlayerEffectsPacket(dm.m_radEffects, 
		dm.m_h2oEffects, dm.m_fodEffects, dm.m_slpEffects, dm.m_playerEffects));
}

void DataManager::updateWorldInfo(void *)
{
	static int usableWidth = 0;
	static int usableHeight = 0;
	static int16_t cellNWX = 0;
	static int16_t cellNWY = 0;
	static int16_t cellSEX = 0;
	static int16_t cellSEY = 0;
	static float scale = 1;
	static float offsetX = 0;
	static float offsetY = 0;
	static std::string mapPath = "";

	const std::string texturePrefix = "textures\\";
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	DataManager &dm = DataManager::getInstance();
	DataHandler *dh = DataHandler::Get();

	TESGlobal *year = (TESGlobal *)LookupFormByID(0x35);
	TESGlobal *month = (TESGlobal *)LookupFormByID(0x36);
	TESGlobal *day = (TESGlobal *)LookupFormByID(0x37);
	TESGlobal *time = (TESGlobal *)LookupFormByID(0x38);

	if (year == NULL || month == NULL || day == NULL || time == NULL || player == NULL ||
		dh == NULL)
		return;

	signed char hour = (signed char)((int)time->data);
	signed char minute = (signed char)((time->data - hour) * 60);

	std::string locationName = "";
	TESObjectCELL *currentCell = player->parentCell;
	if (currentCell != NULL) {
		if (currentCell->worldSpace != NULL && currentCell->worldSpace->fullName.name.m_dataLen > 0) {
			locationName = currentCell->worldSpace->fullName.name.CStr();
		}
		if (currentCell->fullName.name.m_dataLen > 0) {
			locationName = currentCell->fullName.name.CStr();
		}
		if (currentCell->worldSpace != NULL) {
			// Find parent with map data
			TESWorldSpace *worldspace;
			for (worldspace = currentCell->worldSpace; (worldspace->parentFlags & (1 << 2)) != 0 &&
				worldspace->parent != NULL; worldspace = worldspace->parent);
			usableWidth = worldspace->mapData.usableDimensions.X;
			usableHeight = worldspace->mapData.usableDimensions.Y;
			cellNWX = worldspace->mapData.cellNWCoordinates.X;
			cellNWY = worldspace->mapData.cellNWCoordinates.Y;
			cellSEX = worldspace->mapData.cellSECoordinates.X;
			cellSEY = worldspace->mapData.cellSECoordinates.Y;
			if (worldspace->texture.ddsPath.m_dataLen > 0) {
				mapPath = texturePrefix + worldspace->texture.ddsPath.CStr();
			}
			else {
				mapPath = "";
			}
			scale = currentCell->worldSpace->worldMapScale;
			offsetX = currentCell->worldSpace->worldMapCellX;
			offsetY = currentCell->worldSpace->worldMapCellY;
			//scale = worldspace->worldMapScale;
			//offsetX = worldspace->worldMapCellX;
			//offsetY = worldspace->worldMapCellY;
		}
	}


	CommunicationManager::getInstance().sendPacket(new SetWorldInfoPacket((short)year->data, 
		(signed char)month->data, (signed char)day->data, hour, minute, locationName, 
		usableWidth, usableHeight, cellNWX, cellNWY, cellSEX, cellSEY, scale, offsetX, 
		offsetY, mapPath));

}

std::string createAmmoString(TESAmmo *ammo, int currentClip) {
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	if (ammo == NULL || player == NULL)
		return "--";
	std::string result = ammo->shortName.CStr();
	SInt32 numRounds = 0;
	GetItemByRefID(player, ammo->refID, &numRounds);
	result += "(" + std::to_string(currentClip) + "/" + 
		std::to_string(numRounds - currentClip) + ")";
	return result;
}

std::string createAmmoString(BGSListForm *list, int currentClip) {
	TESAmmo *ammo = DYNAMIC_CAST(list->GetNthForm(0), TESForm, TESAmmo);
	return createAmmoString(ammo, currentClip);
}

void DataManager::updateInventory(void *)
{
	const std::string texturePrefix = "textures\\";
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	DataManager &dm = DataManager::getInstance();
	DataHandler *dh = DataHandler::Get();
	BGSDefaultObjectManager *def = BGSDefaultObjectManager::GetSingleton();
	if (def == NULL || dh == NULL || player == NULL)
		return;

	ExtraContainerChanges* pXContainerChanges = static_cast<ExtraContainerChanges*>
		(player->extraDataList.GetByType(kExtraData_ContainerChanges));
	ExtraContainerInfo info(pXContainerChanges ? pXContainerChanges->GetEntryDataList() : NULL);

	for (std::vector<Item *>::iterator it = dm.m_inventory.begin(); it != dm.m_inventory.end();
	++it) {
		delete *it;
	}
	dm.m_inventory.clear();

	for (ExtraDataVec::iterator it = info.m_vec.begin(); it != info.m_vec.end(); ++it) {
		if (!(*it)->type->IsInventoryObject() || (*it)->countDelta <= 0)
			continue;
		ExtraContainerChanges::ExtendDataList *extendData = (*it)->extendData;
		TESForm *object = (*it)->type;
		ExtraHealth *curHealth = NULL;
		int count = (*it)->countDelta;
		bool worn = false;
		for (ExtraContainerChanges::ExtendDataList::Iterator it = extendData->Begin();
			!it.End(); ++it) {
			curHealth = GetByTypeCast((*(*it)), Health);
			worn |= ((BaseExtraList *)(*it))->IsWorn();
		}
		
		int refID = (*it)->type->refID;
		if ((*it)->type->IsReference()) {
			refID = ((TESObjectREFR *)(*it)->type)->baseForm->refID;
		}

		if (refID == def->defaultObjects.asStruct.Stimpak->refID) {
			dm.m_numStimpacks = (*it)->countDelta;
		}
		else if (refID == def->defaultObjects.asStruct.DoctorsBag->refID) {
			dm.m_numDrBag = (*it)->countDelta;
		}
		else if (refID == def->defaultObjects.asStruct.RadAway->refID) {
			dm.m_numRadaway = (*it)->countDelta;
		}
		else if (refID == def->defaultObjects.asStruct.RadX->refID) {
			dm.m_numRadX = (*it)->countDelta;
		}

		switch ((*it)->type->GetTypeID()) {
		case kFormType_Weapon: 
		{
			TESObjectWEAP *weapon = DYNAMIC_CAST(object, TESForm, TESObjectWEAP);
			std::string icon = "";
			if (weapon->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + weapon->icon.ddsPath.CStr();
			float cnd = 1;
			int value = weapon->value.value;
			float dam = weapon->attackDmg.damage * 0.6735;
			if (curHealth != NULL) {
				cnd = curHealth->health / weapon->health.GetHealth();
				value = (int)(value * powf(cnd, 1.5));
				dam *= cnd;
			}
			std::string ammo = "--";
			if (weapon->ammo.ammo != NULL) {
				switch (weapon->ammo.ammo->typeID) {
				case kFormType_Ammo:
					// simple
					ammo = createAmmoString(DYNAMIC_CAST(weapon->ammo.ammo, TESForm, TESAmmo),
						weapon->clipRounds.clipRounds);
					break;
				case kFormType_ListForm:
					ammo = createAmmoString(DYNAMIC_CAST(weapon->ammo.ammo, TESForm, BGSListForm),
						weapon->clipRounds.clipRounds);
					break;
				}
			}
			dm.m_inventory.push_back(new WeaponItem(weapon->refID, weapon->fullName.name.CStr(),
				count, value, weapon->weight.weight, 
				icon, "", true, worn, "", 
				floor(dam*weapon->animShotsPerSec + 0.5f), floorf(dam + 0.5f), 
				cnd, weapon->strRequired, ammo));
			break;
		}
		case kFormType_Armor:
		{
			TESObjectARMO *armor = DYNAMIC_CAST(object, TESForm, TESObjectARMO);
			float cnd = 1;
			int value = armor->value.value;
			if (curHealth != NULL) {
				cnd = curHealth->health / armor->health.GetHealth();
				value = (int)(value * powf(cnd, 1.5));
			}
			std::string icon = "";
			TESIcon *iconPtr = &armor->bipedModel.icon[0];
			if (iconPtr->ddsPath.m_dataLen > 0)
				icon = texturePrefix + iconPtr->ddsPath.CStr();
			std::string armorType = "";
			if((armor->bipedModel.bipedFlags & 0x88) == 0x00) {
				armorType = "Light";
			} else if((armor->bipedModel.bipedFlags & 0x88) == 0x08) {
				armorType = "Medium";
			} else if ((armor->bipedModel.bipedFlags & 0x88) == 0x00) {
				armorType = "Heavy";
			}
			std::string effects = "";
			if (armor->enchantable.enchantItem != NULL) {
				effects = getEffectsString(&armor->enchantable.enchantItem->magicItem.list);
			}
			dm.m_inventory.push_back(new ApparelItem(armor->refID, armor->fullName.name.CStr(),
				count, value, armor->weight.weight, icon, "",
				true, worn, effects, armor->damageThreshold, 0, cnd, armorType));
			break;
		}
		case kFormType_AlchemyItem:
		{
			AlchemyItem *alchemy = DYNAMIC_CAST(object, TESForm, AlchemyItem);
			std::string icon = "";
			if (alchemy->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + alchemy->icon.ddsPath.CStr();
			std::string effects = getEffectsString(&alchemy->effects);
			dm.m_inventory.push_back(new AidItem(alchemy->refID, alchemy->fullName.name.CStr(), count,
				alchemy->value, 0, icon, "", false, worn, effects));
			break;
		}
		case kFormType_ItemMod:
		{
			TESObjectIMOD *itemMod = DYNAMIC_CAST(object, TESForm, TESObjectIMOD);
			std::string icon = "";
			if (itemMod->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + itemMod->icon.ddsPath.CStr();

			char *description = itemMod->description.Get(itemMod, 'CSED');
			if (description == NULL) {
				description = "";
			}

			dm.m_inventory.push_back(new ItemModItem(itemMod->refID, itemMod->name.name.CStr(),
				count, itemMod->value.value, itemMod->weight.weight, icon, "", false,
				false, description));

			break;
		}
		case kFormType_Key:
		case kFormType_Misc:
		{
			TESObjectMISC *misc = DYNAMIC_CAST(object, TESForm, TESObjectMISC);

			std::string icon = "";
			if (misc->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + misc->icon.ddsPath.CStr();

			dm.m_inventory.push_back(new MiscItem(misc->refID, misc->fullName.name.CStr(),
				count, misc->value.value, misc->weight.weight, icon, "", false, false,
				""));
			break;
		}
		case kFormType_Ammo:
		{
			TESAmmo *ammo = DYNAMIC_CAST(object, TESForm, TESAmmo);

			std::string icon = "";
			if (ammo->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + ammo->icon.ddsPath.CStr();

			std::string effects = "";
			for (tList<TESAmmoEffect>::Iterator it = ammo->effectList.Begin();
				!it.End(); ++it) {
				switch (it->type) {
				case TESAmmoEffect::kEffectType_DamageMod:
					effects += "DAM";
					break;
				case TESAmmoEffect::kEffectType_DRMod:
					effects += "Target DR";
					break;
				case TESAmmoEffect::kEffectType_DTMod:
					effects += "Target DT";
					break;
				case TESAmmoEffect::kEffectType_SpreadMod:
					effects += "Gun Spread";
					break;
				case TESAmmoEffect::kEffectType_ConditionMod:
					effects += "Gun CND";
					break;
				case TESAmmoEffect::kEffectType_FatigueMod:
					effects += "Target Fatigue";
					break;
				}
				switch (it->operation) {
				case TESAmmoEffect::kOperation_Add:
					effects += " + ";
					break;
				case TESAmmoEffect::kOperation_Multiply:
					effects += " x ";
					break;
				case TESAmmoEffect::kOperation_Subtract:
					effects += " - ";
					break;
				}

				char tempBuffer[20];
				sprintf_s(tempBuffer, "%.2f", it->value);
				effects += tempBuffer;
				effects += '\n';
			}
			if (effects.length() > 0) {
				// When effects was written to 
				// Also a new line was appended
				// the last new line isn't necessary

				effects.pop_back();
			}

			dm.m_inventory.push_back(new AmmoItem(ammo->refID, ammo->fullName.name.CStr(),
				count, ammo->value.value, ammo->weight, icon, "", false, false, effects));
			break;
		}
		case kFormType_Book:
		{
			TESObjectBOOK *book = DYNAMIC_CAST(object, TESForm, TESObjectBOOK);

			std::string icon = "";
			if (book->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + book->icon.ddsPath.CStr();

			dm.m_inventory.push_back(new AidItem(book->refID, book->fullName.name.CStr(),
				count, book->value.value, book->weight.weight, icon, "", false, false,
				""));
			break;
		}/*
		case kFormType_Clothing:
		{
			break;
		}
		case kFormType_Ingredient:
		{
			break;
		}
		case kFormType_Light:
		{
			break;
		}
		case kFormType_Note:
		{
			break;
		}
		case kFormType_ConstructibleObject:
		{
			break;
		}
		case kFormType_LeveledItem:
		{
			break;
		}*/
		case kFormType_CasinoChip:
		{
			TESCasinoChips *cchip = DYNAMIC_CAST(object, TESForm, TESCasinoChips);
			std::string icon = "";
			if (cchip->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + cchip->icon.ddsPath.CStr();

			dm.m_inventory.push_back(new MiscItem(cchip->refID, cchip->fullName.name.CStr(),
				count, 0, 0, icon, "", false, false, ""));
			break;
		}
		case kFormType_CaravanMoney:
		{
			TESCaravanMoney *cmon = DYNAMIC_CAST(object, TESForm, TESCaravanMoney);

			std::string icon = "";
			if (cmon->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + cmon->icon.ddsPath.CStr();

			dm.m_inventory.push_back(new MiscItem(cmon->refID, cmon->fullName.name.CStr(),
				count, cmon->value.value, 0, icon, "", false, false,
				""));
			break;
		}
		case kFormType_CaravanCard:
		{
			TESCaravanCard *card = DYNAMIC_CAST(object, TESForm, TESCaravanCard);
			std::string icon = "";
			if (card->icon.ddsPath.m_dataLen > 0)
				icon = texturePrefix + card->icon.ddsPath.CStr();

			dm.m_inventory.push_back(new MiscItem(card->refID, card->fullName.name.CStr(),
				count, card->value.value, 0, icon, "", false, false, ""));
			break;
		}
		default:
			_MESSAGE("Unknown item type found in inventory %s(%d): %s(%d)", 
				GetObjectClassName((*it)->type), (*it)->type->GetTypeID(), 
				(*it)->type->GetFullName()->name.CStr(), (*it)->countDelta);
			break;
		}
	}

	//IDebugLog::Outdent();

	CommunicationManager::getInstance().sendPacket(new SetInventoryPacket(dm.m_inventory, false));
}

void DataManager::updateNotes(void *)
{
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	const std::string texturePrefix = "textures\\";
	DataManager &dm = DataManager::getInstance();

	if (player == NULL)
		return;

	for (std::vector<Note *>::iterator it = dm.m_notes.begin(); it != dm.m_notes.end();
	++it) {
		delete *it;
	}
	dm.m_notes.clear();

	for (PlayerCharacter::NoteListEntry *entry = &player->noteList; entry != NULL; 
			entry = entry->nextNote) {
		BGSNote *note = entry->note;
		if (note == NULL)
			continue;
		const char *title = note->fullName.name.CStr();
		switch (CALL_MEMBER_FN(note, GetDataType)()) {
		case BGSNote::Sound:
		{
			UInt32 soundID = CALL_MEMBER_FN(note, GetSoundNote)();
			dm.m_notes.push_back(new Note(title, NOTETYPE_SOUND, "", note->flags));
			break;
		}
		case BGSNote::Text:
		{
			TESDescription *desc = CALL_MEMBER_FN(note, GetTextNote)();
			const char *text = "";
			if(desc)
				text = desc->Get(note, 'MANT');
			dm.m_notes.push_back(new Note(title, NOTETYPE_TEXT, text, note->flags));
			break;
		}
		case BGSNote::Image:
		{
			TESIcon *image = CALL_MEMBER_FN(note, GetImageNote)();

			std::string path = "";
			if (image != NULL && image->ddsPath.CStr() != NULL)
				path = texturePrefix + image->ddsPath.CStr();

			dm.m_notes.push_back(new Note(title, NOTETYPE_IMAGE, path, note->flags));
			break;
		}
		case BGSNote::Voice:
		{
			UInt32 topicID = CALL_MEMBER_FN(note, GetVoiceNoteTopic)();
			TESNPC *speakerID = CALL_MEMBER_FN(note, GetVoiceNoteSpeaker)();
			dm.m_notes.push_back(new Note(title, NOTETYPE_VOICE, "", note->flags));
			break;
		}
		}
	}


	CommunicationManager::getInstance().sendPacket(new SetNotesPacket(dm.m_notes, false));
}

void DataManager::updateQuests(void *)
{
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	DataManager &dm = DataManager::getInstance();

	if (player == NULL)
		return;

	std::map<TESQuest *, Quest *> quests;
	std::list<TESQuest *> order;
	
	for (tList<BGSQuestObjective>::Iterator it = player->questObjectiveList.Begin();
		!it.End(); ++it) {
		if (quests.find(it->quest) == quests.end()) {
			quests[it->quest] = new Quest(it->quest->refID, it->quest->fullName.name.CStr(), 0);
			order.push_back(it->quest);
			quests[it->quest]->setActive(it->quest == player->quest);
			quests[it->quest]->setCompleted((it->quest->flags & 0x02) != 0);
		}
		Quest *quest = quests[it->quest];
		const char *text = "";
		String *textStr = const_cast<String *>(&it->displayText);
		if (textStr->CStr() != NULL)
			text = textStr->CStr();
		QuestObjective objective(it->objectiveId, text, 0);
		objective.setCompleted((it->status & BGSQuestObjective::eQObjStatus_completed) != 0);
		objective.setDisplayed((it->status & BGSQuestObjective::eQObjStatus_displayed) != 0);
		for (tList<BGSQuestObjective::Target *>::Iterator it2 = it->targets.Begin(); !it2.End(); 
			++it2) {
			if (**it2 <= (void*)0x10000 || (**it2)->target == NULL) continue;
			TESObjectREFR *targetRef = (**it2)->target->target;
			if (targetRef == NULL) {
				continue;
			}
			QuestObjective::Target target;
			target.id = targetRef->refID;
			target.x = targetRef->posX;
			target.y = targetRef->posY;
			target.z = targetRef->posZ;
			objective.addTarget(target);
		}
		quest->addObjective(objective);
	}


	for (std::vector<Quest *>::iterator it = dm.m_quests.begin(); it != dm.m_quests.end();
	++it) {
		delete *it;
	}
	dm.m_quests.clear();
	for (std::list<TESQuest *>::iterator it = order.begin(); it != order.end(); ++it) {
		Quest *quest = quests.at(*it);
		if(!quest->isCompleted())
			dm.m_quests.push_back(quest);
	}
	for (std::list<TESQuest *>::iterator it = order.begin(); it != order.end(); ++it) {
		Quest *quest = quests.at(*it);
		if (quest->isCompleted())
			dm.m_quests.push_back(quest);
	}

	CommunicationManager::getInstance().sendPacket(new SetQuestsPacket(dm.m_quests, false));
}

void DataManager::updateMapMarkers(void *)
{
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	DataManager &dm = DataManager::getInstance();
	if (player == NULL) return;
	TESObjectCELL *currentCell = player->parentCell;
	if (currentCell == NULL || currentCell->worldSpace == NULL || 
		currentCell->worldSpace->cell == NULL) {
		return;
	}
	
	for (std::vector<MapMarker *>::iterator it = dm.m_markers.begin(); it != dm.m_markers.end();
		++it) {
		delete *it;
	}
	dm.m_markers.clear();

	TESObjectCELL *cell = currentCell->worldSpace->cell;
	for (tList<TESObjectREFR>::Iterator it = cell->objectList.Begin(); !it.End(); ++it) {
		const char *name = "";
		const char *className = GetObjectClassName(it->baseForm);
		if (it->baseForm->GetFullName() != NULL) {
			name = it->baseForm->GetFullName()->name.CStr();
		}
		if (className == NULL) {
			className = "";
		}
		//_MESSAGE("Object: %08X base:%08X class:(%s) name:(%s)", it->refID, it->baseForm->refID, className, name);
		if (it->baseForm == *((TESForm **)0x11CA224)) { // MapMarker Reference
			ExtraMapMarker *extraMarker = 
				(ExtraMapMarker *)it->extraDataList.GetByType(kExtraData_MapMarker);
			if (extraMarker != NULL && extraMarker->data != NULL) {
				TESReputation *reputation = DYNAMIC_CAST(extraMarker->data->reputation, 
					TESForm, TESReputation);
				std::string reputationStr;
				if (reputation != NULL) {
				}
				dm.m_markers.push_back(new MapMarker(it->refID, it->posX, it->posY, it->posZ,
					(enum MapMarker::Type) extraMarker->data->type,
					extraMarker->data->fullName.name.CStr(), extraMarker->data->flags, 
					reputationStr));
			}
		}
	}
	CommunicationManager::getInstance().sendPacket(new SetMapMarkersPacket(dm.m_markers, false));
}

void memdump(const void *ptr, int size) {
	const unsigned char *ptr2 = (const unsigned char *)ptr;
	for (int i = 0; i < size; i += 8) {
		_MESSAGE("%02x %02x %02x %02x %02x %02x %02x %02x", ptr2[0], ptr2[1], ptr2[2], ptr2[3], ptr2[4], ptr2[5], ptr2[6], ptr2[7]);
		ptr2 += 8;
	}
}

void DataManager::updateRadio(void *) {
	struct currentRadio_s{
		TESObjectREFR *ref;
	};
	DataManager &dm = DataManager::getInstance();
	PlayerCharacter *player = PlayerCharacter::GetSingleton();
	char *currentSong = (char *)0x11DD448;
	struct currentRadio_s **currentRadio = (struct currentRadio_s **)0x11DD42C;

	dm.m_radioStations.clear();
	//_MESSAGE("Radio stations %08x base %08x", 
	//		(*currentRadio != NULL && (*currentRadio)->ref != NULL) ? (*currentRadio)->ref->refID : 0, 
	//	(*currentRadio != NULL && (*currentRadio)->ref != NULL && 
	//		(*currentRadio)->ref->baseForm != NULL) ? (*currentRadio)->ref->baseForm->refID : 0);
	for (std::set<UInt32>::iterator it = dm.m_radioIds.getList().cbegin();
		it != dm.m_radioIds.getList().cend(); ++it) {
		TESObjectREFR *ref = DYNAMIC_CAST(LookupFormByID(*it), TESForm, TESObjectREFR);
		if (ref == NULL) {
			continue;
		}
		double disabled = 0;
		if (!isDisabled(ref, NULL, NULL, &disabled)) {
			disabled = 0;
		}

		BGSTalkingActivator *acti = DYNAMIC_CAST(ref->baseForm, TESForm, BGSTalkingActivator);
		if (acti == NULL || !acti->isRadio() || acti->isNonPipboy() || disabled != 0) continue;
		std::string name = acti->fullName.name.CStr();
		ExtraRadioData *radioData = (ExtraRadioData *)ref->extraDataList.GetByType(kExtraData_RadioData);
		bool active = true;
		bool current = false;
		if (radioData != NULL) {
			switch (radioData->mode) {
			case ExtraRadioData::kModeRadius:
				{
					float dx, dy, dz;
					if (radioData->exteriorPosRef != NULL) {
						dx = radioData->exteriorPosRef->posX - player->posX;
						dy = radioData->exteriorPosRef->posY - player->posY;
						dz = radioData->exteriorPosRef->posZ - player->posZ;
					}
					else {
						dx = ref->posX - player->posX;
						dy = ref->posY - player->posY;
						dz = ref->posZ - player->posZ;
					}
					float distance = sqrtf(dx * dx + dy * dy + dz * dz);
					if (distance >= radioData->radius) {
						active = false;
					}
				}
				break;
			case ExtraRadioData::kModeWorldspaceAndLinkedInteriors:
				{

				}
				break;
			case ExtraRadioData::kModeCurrentCellOnly:
				{

				}
				break;
			case ExtraRadioData::kModeLinkedInteriors:
				{

				}
				break;
			}
		}

		if (*currentRadio != NULL && (*currentRadio)->ref != NULL) {
			current = (*currentRadio)->ref->refID == ref->refID;
		}

		dm.m_radioStations.push_back(Radio(ref->refID, name, active, current));
		//_MESSAGE("Radio %40s: id: %08x base: %08x active: %d current: %d song: %s", name.c_str(), 
		//			ref->refID, ref->baseForm->refID, active, current, currentSong);
	}
	CommunicationManager::getInstance().sendPacket(new SetRadioPacket(dm.m_radioStations));
}

std::string DataManager::getSystemName()
{
	char nameBuffer[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD bufferLen = MAX_COMPUTERNAME_LENGTH + 1;
	if (!GetComputerNameA(nameBuffer, &bufferLen)) {
		return "Pipboy 3000";
	}
	return std::string(nameBuffer);
}
