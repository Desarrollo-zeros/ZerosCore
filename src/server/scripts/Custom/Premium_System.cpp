
#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "MapManager.h"
#include "Chat.h"
#include "Common.h"
#include "Language.h"
#include "CellImpl.h"
#include "GridNotifiers.h"

class premiumcommands : public CommandScript
{
public:
    premiumcommands() : CommandScript("premiumcommands") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand premiumCommandTable[] =

        {
		{ "add",            SEC_PREMIUM,  false, &HandleAddItemCommand,	 	     "", NULL },
		{ "activar",        SEC_PLAYER,  false, &HandleActivarCommand,		 "", NULL },
 
        { NULL,             0,                     false, NULL,                  "", NULL }
        };
        static ChatCommand commandTable[] =
        {
           { "premium",	    SEC_PLAYER,   true, NULL,                     "",  premiumCommandTable},
	       { NULL,             0,                  false, NULL,                  "", NULL }
        };
        return commandTable;
    }

static bool HandleActivarCommand(ChatHandler * handler, const char * args)
{
	Player* player = handler->GetSession()->GetPlayer();

	if(player->GetSession()->GetSecurity() >= 2)
	{
		handler->PSendSysMessage("BIENVENIDO! Ahora eres PREMIUM.");
		handler->SetSentErrorMessage(true);
		return false;
	}

	if (player->HasItemCount(600002, 1, false)) 
	{
                  PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_ACCESS);
                  stmt->setUInt32(0, player->GetSession()->GetAccountId());
                  stmt->setUInt8(1, 1);
                  stmt->setInt32(2, 1);
	         LoginDatabase.Execute(stmt);
		player->DestroyItemCount(600002, 1, true, false); 
		handler->PSendSysMessage("¡Tu PREMIUM se ha Activado!, ¡Cierra el juego y entra de nuevo para completar la activacion!");
		return true;
	}
	return true;
}
static bool HandleAddItemCommand(ChatHandler* handler, const char* args)

    {
        if (!*args)
            return false;
		
		 Player* me = handler->GetSession()->GetPlayer();
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
            player->DestroyItemCount(itemId, -count, true, false);
            handler->PSendSysMessage(LANG_REMOVEITEM, itemId, -count, handler->GetNameLink(player).c_str());
            return true;
        }

        // Adding items
        uint32 noSpaceForCount = 0;

        // check space and find places
        ItemPosCountVec dest;
        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
            count -= noSpaceForCount;

        if (count == 0 || dest.empty())                         // can't add any
        {
            handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);
            handler->SetSentErrorMessage(true);
            return false;
		}

		if (itemId == 600007 || itemId == 600006 || itemId == 600005 || itemId == 600004 || itemId == 600003 || itemId == 34597 || itemId == 37829 || itemId == 900000 || itemId == 7000000 || itemId == 900000 || itemId == 7000002 || itemId == 7000008 || itemId == 41426 || itemId == 46104 )          // Items prohibidos
		{
			handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);
			handler->SetSentErrorMessage(true);
			return false;
		}
			

        Item* item = player->StoreNewItem(dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));

        // remove binding (let GM give it to another player later)
            for (ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); ++itr)
                if (Item* item1 = player->GetItemByPos(itr->pos))
                    item1->SetBinding(true);

        if (count > 0 && item)
        {
            player->SendNewItem(item, count, false, true);
                player->SendNewItem(item, count, true, false);
        }

        if (noSpaceForCount > 0)
            handler->PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);

        return true;
    }
static bool HandlepremiumCommand(ChatHandler* handler, const char* args)
    {

        Player* me = handler->GetSession()->GetPlayer();

            me->Say("premium command?", LANG_UNIVERSAL);
            return true;
    }
	
};

void AddSC_premiumcommands()
{
    new premiumcommands();
}