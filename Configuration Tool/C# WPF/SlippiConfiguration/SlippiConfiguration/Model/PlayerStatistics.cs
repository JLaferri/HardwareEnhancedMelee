using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Fizzi.Applications.SlippiConfiguration.Common;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class PlayerStatistics : INotifyPropertyChanged
    {
        //Set up backing fields for the fields that we want to appear on the UI
        public UInt32 _actionCount;
        public float _averageEdgeguardDamage;
        public float _averageComboStringDamage;

        public UInt32 ActionCount { get { return _actionCount; } set { this.RaiseAndSetIfChanged("ActionCount", ref _actionCount, value, PropertyChanged); } }
        public float AverageEdgeguardDamage { get { return _averageEdgeguardDamage; } set { this.RaiseAndSetIfChanged("AverageEdgeguardDamage", ref _averageEdgeguardDamage, value, PropertyChanged); } }
        public float AverageComboStringDamage { get { return _averageComboStringDamage; } set { this.RaiseAndSetIfChanged("AverageComboStringDamage", ref _averageComboStringDamage, value, PropertyChanged); } }

        private List<StockStatistics> stocks;
        public StockStatistics[] Stocks { get { return stocks.ToArray(); } }

        public StockStatistics CurrentStock { get; private set; }

        private List<ComboString> comboStrings;
        public ComboString[] ComboStrings { get { return comboStrings.ToArray(); } }

        private List<Recovery> recoveries;
        public Recovery[] Recoveries { get { return recoveries.ToArray(); } }

        public PlayerStatistics()
        {
            stocks = new List<StockStatistics>();
            CurrentStock = new StockStatistics();
            stocks.Add(CurrentStock);

            comboStrings = new List<ComboString>();
            recoveries = new List<Recovery>();
        }

        public void AddComboString(int hitCount, uint startFrame, uint endFrame, float startPercent, float endPercent)
        {
            comboStrings.Add(new ComboString(hitCount, startFrame, endFrame, startPercent, endPercent));
            AverageComboStringDamage = comboStrings.Average(cs => cs.EndPercent - cs.StartPercent);
        }

        internal void AddRecovery(bool isSuccessful, uint startFrame, uint endFrame, float startPercent, float endPercent)
        {
            recoveries.Add(new Recovery(isSuccessful, startFrame, endFrame, startPercent, endPercent));
            AverageEdgeguardDamage = recoveries.Average(r => r.EndPercent - r.StartPercent);
        }

        public event PropertyChangedEventHandler PropertyChanged;

        
    }
}
