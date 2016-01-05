using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    static class ControllerHelpers
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
    }
}
