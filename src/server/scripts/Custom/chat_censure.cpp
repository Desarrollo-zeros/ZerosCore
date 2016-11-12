
#include "ScriptPCH.h"
#include "Channel.h"
#include "Language.h"
#include <string>
 
class System_Censure : public PlayerScript
{
public:
        System_Censure() : PlayerScript("System_Censure") {}
 
        void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg)
        {
                CheckMessage(player, msg, lang, NULL, NULL, NULL, NULL);
        }
 
        void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Player* receiver)
        {
                CheckMessage(player, msg, lang, receiver, NULL, NULL, NULL);
        }
 
        void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Group* group)
        {
                CheckMessage(player, msg, lang, NULL, group, NULL, NULL);
        }
 
        void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Guild* guild)
        {
                CheckMessage(player, msg, lang, NULL, NULL, guild, NULL);
        }
 
        void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Channel* channel)
        {
                CheckMessage(player, msg, lang, NULL, NULL, NULL, channel);
        }
 
void CheckMessage(Player* player, std::string& msg, uint32 lang, Player* /*receiver*/, Group* /*group*/, Guild* /*guild*/, Channel* channel)
{
    if (player->isGameMaster() || lang == LANG_ADDON)
            return;
 
    //transform to lowercase (for simpler checking)
    std::string lower = msg;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
 
        const uint8 cheksSize = 20;
        std::string checks[cheksSize];
        checks[0] ="http://";
        checks[1] =".com";
        checks[2] =".net";
        checks[3] =".org";
        checks[4] =".ru";
        checks[5] ="rondor";
        checks[6] ="deathforce";
		checks[7] ="d e a t h f o r c e";
		checks[8] ="death force";
		checks[9] ="deathf";
		checks[10] ="deat hforce";
		checks[11] ="deathfor ce";
		checks[12] ="de athforce";
		checks[13] ="d eathforce";
		checks[14] ="force";
		checks[15] ="de athforce";
        checks[16] ="wow colombia";
        checks[17] ="wow co";
        checks[18] ="monster";
        checks[19] ="firestorm";
		checks[20] ="global";
		checks[21] ="nexus";
		checks[22] ="globalwow";
		checks[23] ="ultra";
		checks[24] ="ice";






			
        for (int i = 0; i < cheksSize; ++i)
            if (lower.find(checks[i]) != std::string::npos)
            {
                msg = "No se tolera el Spam!";
			    msg = lang == LANG_GM_SILENCE;
				ChatHandler(player->GetSession()).PSendSysMessage("No se tolera el Spam!");
				std::string plr = player->GetName();
            std::string gender = player->getGender() == GENDER_FEMALE ? "haciendo" : "haciendo";
            bool ingroup = player->GetGroup();
            std::string tag_colour = "7bbef7";
            std::string plr_colour = "ff0000";
            std::string boss_colour = "18be00";
            std::ostringstream stream;
            stream << "|CFF" << tag_colour << 
                      "[Anuncio de Spam ]|r:|cff" << plr_colour << " " << plr << 
                      (ingroup ? " el " : " esta ") << gender << (ingroup ? " Spam " : " Spam ") << "!";
			sWorld->SendGMText(LANG_GM_BROADCAST, stream.str().c_str());
                return;
            }
    
	}  
    };

void AddSC_System_Censure()
{
    new System_Censure();
}
