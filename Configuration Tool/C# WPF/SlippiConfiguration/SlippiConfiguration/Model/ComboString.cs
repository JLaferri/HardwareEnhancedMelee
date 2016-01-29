using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Fizzi.Applications.SlippiConfiguration.Common;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class ComboString : INotifyPropertyChanged
    {
        //Set up backing fields for the fields that we want to appear on the UI
        private int _hitCount;

        public int HitCount { get { return _hitCount; } set { this.RaiseAndSetIfChanged("HitCount", ref _hitCount, value, PropertyChanged); } }
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

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
