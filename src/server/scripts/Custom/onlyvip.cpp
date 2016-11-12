
#include "ScriptPCH.h"
 
class Vip_Access: public PlayerScript
{
    public:
        Vip_Access() : PlayerScript("Vip_Access") {}
 
    void OnUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/)
    {
    	if (player->GetAreaId() == 9999 && player->GetSession()->GetSecurity() == SEC_PLAYER)
    	{
		player->TeleportTo(732, -286, 1372, 22, 6);
		ChatHandler(player->GetSession()).PSendSysMessage("|cffff6060[Information]:|r No tienes permitido estar aqui, no eres un VIP!|r!");
    	}
	}
};
 
void AddSC_Vip_Access()
{
    new Vip_Access();
}