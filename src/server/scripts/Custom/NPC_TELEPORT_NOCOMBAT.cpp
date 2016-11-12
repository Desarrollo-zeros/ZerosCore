
#include "ScriptPCH.h"



class npc_nofunciona_encombate : public CreatureScript
{
public:
	npc_nofunciona_encombate() : CreatureScript("npc_nofunciona_encombate") { }

	bool OnGossipHello(Player* pPlayer, Creature* pCreature)

	{
		if (pPlayer->isInCombat())
		{
			pPlayer->GetSession()->SendNotification("¡COBARDE ESTAS EN COMBATE!");
			return true;
		}

	}
};


void AddSC_global_teleporter()
{
	new npc_nofunciona_encombate();
}