using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class PlayerFlags
    {
        public bool IsRecovering { get; set; }
        public bool IsHitOffStage { get; set; }
        public bool IsLandedOnStage { get; set; }
        public int FramesSinceLanding { get; set; }
        public float RecoveryStartPercent { get; set; }
        public uint RecoveryStartFrame { get; set; }

        //Combo string stuff
        public int StringCount { get; set; }
        public float StringStartPercent { get; set; }
        public uint StringStartFrame { get; set; }
        public int StringResetCounter { get; set; }

        public void ResetRecoveryFlags()
        {
            IsRecovering = false;
            IsHitOffStage = false;
            IsLandedOnStage = false;
            FramesSinceLanding = 0;
        }
    }
}
