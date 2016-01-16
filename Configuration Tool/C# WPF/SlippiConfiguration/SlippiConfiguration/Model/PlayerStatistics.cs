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
        private UInt32 _actionCount;
        private float _averageEdgeguardDamage;
        private float _averageComboStringDamage;
        private ComboString _currentComboString;
        private Recovery _currentRecovery;

        public UInt32 ActionCount { get { return _actionCount; } set { this.RaiseAndSetIfChanged("ActionCount", ref _actionCount, value, PropertyChanged); } }
        public float AverageEdgeguardDamage { get { return _averageEdgeguardDamage; } set { this.RaiseAndSetIfChanged("AverageEdgeguardDamage", ref _averageEdgeguardDamage, value, PropertyChanged); } }
        public float AverageComboStringDamage { get { return _averageComboStringDamage; } set { this.RaiseAndSetIfChanged("AverageComboStringDamage", ref _averageComboStringDamage, value, PropertyChanged); } }

        private List<StockStatistics> stocks;
        public StockStatistics[] Stocks { get { return stocks.ToArray(); } }

        public StockStatistics CurrentStock { get; private set; }

        private List<ComboString> comboStrings;
        public ComboString[] ComboStrings { get { return comboStrings.ToArray(); } }

        public ComboString CurrentComboString { get { return _currentComboString; } set { this.RaiseAndSetIfChanged("CurrentComboString", ref _currentComboString, value, PropertyChanged); } }

        private List<Recovery> recoveries;
        public Recovery[] Recoveries { get { return recoveries.ToArray(); } }

        public Recovery CurrentRecovery { get { return _currentRecovery; } set { this.RaiseAndSetIfChanged("CurrentRecovery", ref _currentRecovery, value, PropertyChanged); } }

        public PlayerStatistics()
        {
            stocks = new List<StockStatistics>();
            CurrentStock = new StockStatistics();
            stocks.Add(CurrentStock);

            comboStrings = new List<ComboString>();
            recoveries = new List<Recovery>();
        }

        public void NewComboString(uint startFrame, float startPercent)
        {
            CurrentComboString = new ComboString(startFrame, startPercent);
            comboStrings.Add(CurrentComboString);
        }

        public void EndComboString()
        {
            if (CurrentComboString != null)
            {
                CurrentComboString.IsComboFinished = true;
                CurrentComboString = null;
            }
        }

        public void AddRecovery(bool isSuccessful, uint startFrame, uint endFrame, float startPercent, float endPercent)
        {
            recoveries.Add(new Recovery(isSuccessful, startFrame, endFrame, startPercent, endPercent));
            AverageEdgeguardDamage = recoveries.Average(r => r.EndPercent - r.StartPercent);
        }

        public event PropertyChangedEventHandler PropertyChanged;

        
    }
}
