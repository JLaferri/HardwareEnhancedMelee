using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    static class GameStateHelpers
    {
        //Return joystick region
        public static JoystickRegion GetJoystickRegion(float x, float y)
        {
            if (x >= 0.2875 && y >= 0.2875) return JoystickRegion.NE;
            else if (x >= 0.2875 && y <= -0.2875) return JoystickRegion.SE;
            else if (x <= -0.2875 && y <= -0.2875) return JoystickRegion.SW;
            else if (x <= -0.2875 && y >= 0.2875) return JoystickRegion.NW;
            else if (y >= 0.2875) return JoystickRegion.N;
            else if (x >= 0.2875) return JoystickRegion.E;
            else if (y <= -0.2875) return JoystickRegion.S;
            else if (x <= -0.2875) return JoystickRegion.W;
            else return JoystickRegion.DeadZone;
        }

        //Checks if player is off stage
        public static bool CheckIfOffStage(Stage stage, float x, float y)
        {
            //Checks if player is off stage. These are the edge coordinates +5
            switch (stage)
            {
                case Stage.FountainOfDreams:
                    return x < -68.35 || x > 68.35 || y < -10;
                case Stage.PokemonStadium:
                    return x < -92.75 || x > 92.75 || y < -10;
                case Stage.YoshisStory:
                    return x < -61 || x > 61 || y < -10;
                case Stage.DreamLand:
                    return x < -82.27 || x > 82.27 || y < -10;
                case Stage.Battlefield:
                    return x < -73.4 || x > 73.4 || y < -10;
                case Stage.FinalDestination:
                    return x < -90.5606 || x > 90.5606 || y < -10;
                default:
                    return false;
            }
        }
    }
}
