using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class Recovery
    {
        public bool IsSuccessful { get; private set; }
        public uint StartFrame { get; private set; }
        public uint EndFrame { get; private set; }
        public float StartPercent { get; private set; }
        public float EndPercent { get; private set; }

        public Recovery(bool isSuccessful, uint startFrame, uint endFrame, float startPercent, float endPercent)
        {
            IsSuccessful = isSuccessful;
            StartFrame = startFrame;
            EndFrame = endFrame;
            StartPercent = startPercent;
            EndPercent = endPercent;
        }
    }
}
