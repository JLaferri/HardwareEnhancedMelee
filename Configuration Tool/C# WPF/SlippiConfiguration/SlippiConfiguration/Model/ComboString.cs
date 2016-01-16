using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class ComboString
    {
        public int HitCount { get; set; }
        public uint StartFrame { get; private set; }
        public uint EndFrame { get; set; }
        public float StartPercent { get; private set; }
        public float EndPercent { get; set; }
        public bool IsComboFinished { get; set; }

        public ComboString(uint startFrame, float startPercent)
        {
            HitCount = 1;
            StartFrame = startFrame;
            EndFrame = startFrame;
            StartPercent = startPercent;
            EndPercent = startPercent;
            IsComboFinished = false;
        }
    }
}
