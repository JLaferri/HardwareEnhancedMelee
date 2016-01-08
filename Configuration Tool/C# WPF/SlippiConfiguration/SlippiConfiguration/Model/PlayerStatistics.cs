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

        public UInt32 ActionCount { get { return _actionCount; } set { this.RaiseAndSetIfChanged("ActionCount", ref _actionCount, value, PropertyChanged); } }

        private List<StockStatistics> stocks;
        public StockStatistics[] Stocks { get { return stocks.ToArray(); } }

        public StockStatistics CurrentStock { get; private set; }

        public PlayerStatistics()
        {
            stocks = new List<StockStatistics>();
            CurrentStock = new StockStatistics();
            stocks.Add(CurrentStock);
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
