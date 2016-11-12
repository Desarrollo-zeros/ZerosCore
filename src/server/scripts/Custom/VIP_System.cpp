


#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "MapManager.h"
#include "Chat.h"
#include "Common.h"
#include "Language.h"
#include "CellImpl.h"
#include "GridNotifiers.h"

class vipcommands : public CommandScript
{
public:
    vipcommands() : CommandScript("vipcommands") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand vipCommandTable[] =

        {
        { "cambiarraza",      SEC_VIP,  false, &HandleCambiarRazaCommand,       "", NULL },
	    { "cambiarfaccion",	 SEC_VIP,  false, &HandleCambiarFaccionCommand,    "", NULL },
	    { "customizar",       SEC_VIP,  false, &HandlePersonalizarCommand,		 "", NULL },
		{ "activar",        SEC_PLAYER,  false, &HandleActivateCommand,		 "", NULL },
		{ "tele",            SEC_VIP,  false, &HandleTeleCommand,		"", NULL },
		{ "cambiarsexo",			 SEC_VIP,     false, &HandleGenderCommand,        "", NULL },
		{ "banco",			 SEC_VIP,     false, &HandleBancoCommand,        "", NULL },
		{ "demorph",		 SEC_VIP,     false, &HandleDeMorphCommand,        "", NULL },
		{ "morph",			 SEC_VIP,     false, &HandleMorphCommand,        "", NULL },
 
        { NULL,             0,                     false, NULL,                  "", NULL }
        };
        static ChatCommand commandTable[] =
        {
           { "vip",	    SEC_PLAYER,   true, NULL,                     "",  vipCommandTable},
	       { NULL,             0,                  false, NULL,                  "", NULL }
        };
        return commandTable;
    }

static bool HandleActivateCommand(ChatHandler * handler, const char * args)
{
	Player* player = handler->GetSession()->GetPlayer();

	if(player->GetSession()->GetSecurity() >= 1)
	{
		handler->PSendSysMessage("BIENVENIDO! Ahora eres VIP.");
		handler->SetSentErrorMessage(true);
		return false;
	}

	if (player->HasItemCount(600001, 1, false)) 
	{
                  PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_ACCESS);
                  stmt->setUInt32(0, player->GetSession()->GetAccountId());
                  stmt->setUInt8(1, 1);
                  stmt->setInt32(2, 1);
	         LoginDatabase.Execute(stmt);
		player->DestroyItemCount(600001, 1, true, false); 
		handler->PSendSysMessage("¡Tu VIP se ha Activado!, ¡Cierra el juego y entra de nuevo para completar la activacion!");
		return true;
	}
	return true;
}

static bool HandleDeMorphCommand(ChatHandler* handler, const char* /*args*/)
{
	Player* target = handler->GetSession()->GetPlayer();

	target->DeMorph();
	target->RemoveAura(36899);
	target->RemoveAura(36897);

	return true;
}

static bool HandleBancoCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->GetSession()->SendShowBank(handler->GetSession()->GetPlayer()->GetGUID());
        return true;
    }

static bool HandleTeleCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;

        Player* me = handler->GetSession()->GetPlayer();

        // id, or string, or [name] Shift-click form |color|Htele:id|h[name]|h|r
        GameTele const* tele = handler->extractGameTeleFromLink((char*)args);

        if (!tele)
        {
            handler->SendSysMessage(LANG_COMMAND_TELE_NOTFOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (me->isInCombat())
        {
            handler->SendSysMessage(LANG_YOU_IN_COMBAT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        MapEntry const* map = sMapStore.LookupEntry(tele->mapId);
        if (!map || map->IsBattlegroundOrArena())
        {
            handler->SendSysMessage(LANG_CANNOT_TELE_TO_BG);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // stop flight if need
        if (me->isInFlight())
        {
            me->GetMotionMaster()->MovementExpired();
            me->CleanupAfterTaxiFlight();
        }
        // save only in non-flight case
        else
            me->SaveRecallPosition();

        me->TeleportTo(tele->mapId, tele->position_x, tele->position_y, tele->position_z, tele->orientation);
        return true;
		}
static bool HandleMorphCommand(ChatHandler * handler, const char* args)
	{
        Player * pl = handler->GetSession()->GetPlayer();
		
		if(pl->InArena())
		{
			pl->GetSession()->SendNotification("No puedes usar esto en arenas!");
			return false;
		}
		if (strncmp(args, "ali", 3) == 0)
		{
		pl->RemoveAura(36899);
		pl->AddAura(36899, pl);
		return true;
		}
		else if (strncmp(args, "horda", 4) == 0)
		{
		pl->RemoveAura(36897);
		pl->AddAura(36897, pl);
		return true;
		}

    }

static bool HandlevipCommand(ChatHandler* handler, const char* args)
    {

        Player* me = handler->GetSession()->GetPlayer();

            me->Say("vip command?", LANG_UNIVERSAL);
            return true;
    }

static bool HandleGenderCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;

		 Player* target = handler->GetSession()->GetPlayer();

        if (!target)
        {
            handler->PSendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PlayerInfo const* info = sObjectMgr->GetPlayerInfo(target->getRace(), target->getClass());
        if (!info)
            return false;

        char const* gender_str = (char*)args;
        int gender_len = strlen(gender_str);

        Gender gender;

        if (!strncmp(gender_str, "male", gender_len))            // MALE
        {
            if (target->getGender() == GENDER_MALE)
                return true;

            gender = GENDER_MALE;
        }
        else if (!strncmp(gender_str, "female", gender_len))    // FEMALE
        {
            if (target->getGender() == GENDER_FEMALE)
                return true;

            gender = GENDER_FEMALE;
        }
        else
        {
            handler->SendSysMessage(LANG_MUST_MALE_OR_FEMALE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Set gender
        target->SetByteValue(UNIT_FIELD_BYTES_0, 2, gender);
        target->SetByteValue(PLAYER_BYTES_3, 0, gender);

        // Change display ID
        target->InitDisplayIds();

        char const* gender_full = gender ? "female" : "male";

        handler->PSendSysMessage(LANG_YOU_CHANGE_GENDER, handler->GetNameLink(target).c_str(), gender_full);

        if (handler->needReportToTarget(target))
            ChatHandler(target->GetSession()).PSendSysMessage(LANG_YOUR_GENDER_CHANGED, gender_full, handler->GetNameLink().c_str());

        return true;
    }

static bool HandleCambiarRazaCommand(ChatHandler* handler, const char* args)
    {

        Player* me = handler->GetSession()->GetPlayer();
		me->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
		handler->PSendSysMessage("Relogea para tu cambio de raza.");
        return true;
    }

static bool HandleCambiarFaccionCommand(ChatHandler* handler, const char* args)
    {

        Player* me = handler->GetSession()->GetPlayer();
		me->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
		handler->PSendSysMessage("Relogea para tu cambio de faccion.");
        return true;
    }

static bool HandlePersonalizarCommand(ChatHandler* handler, const char* args)
    {

        Player* me = handler->GetSession()->GetPlayer();
		me->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);
		handler->PSendSysMessage("Relogea para perzonalizar tu personaje.");
        return true;
    }

	
};

void AddSC_vipcommands()
{
    new vipcommands();
}