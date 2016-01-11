using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class ComboString
    {
        public int HitCount { get; private set; }
        public uint StartFrame { get; private set; }
        public uint EndFrame { get; private set; }
        public float StartPercent { get; private set; }
        public float EndPercent { get; private set; }

        public ComboString(int hitCount, uint startFrame, uint endFrame, float startPercent, float endPercent)
        {
            HitCount = hitCount;
            StartFrame = startFrame;
            EndFrame = endFrame;
            StartPercent = startPercent;
            EndPercent = endPercent;
        }
    }
}
