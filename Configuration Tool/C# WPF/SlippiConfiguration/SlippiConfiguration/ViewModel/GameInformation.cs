using Fizzi.Applications.SlippiConfiguration.Model;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Fizzi.Applications.SlippiConfiguration.Common;
using System.ComponentModel;

namespace Fizzi.Applications.SlippiConfiguration.ViewModel
{
    class GameInformation : INotifyPropertyChanged
    {
        private Game _currentGame;
        public Game CurrentGame { get { return _currentGame; } private set { this.RaiseAndSetIfChanged("CurrentGame", ref _currentGame, value, PropertyChanged); } }

        public void ProcessMessage(byte[] message)
        {
            if (message.Length <= 0) return;

            switch (message[0])
            {
                case (byte)TcpMessageType.GameStart:
                    handleGameStart(message);
                    break;
                case (byte)TcpMessageType.GameUpdate:
                    handleGameUpdate(message);
                    computeStatistics();
                    break;
                case (byte)TcpMessageType.GameEnd:
                    handleGameEnd(message);
                    break;
            }
        }

        private void handleGameStart(byte[] message)
        {
            CurrentGame = Game.New(message);
        }

        private void handleGameUpdate(byte[] message)
        {
            if (CurrentGame != null) CurrentGame.Update(message);
        }

        private void handleGameEnd(byte[] message)
        {
            if (CurrentGame != null) CurrentGame.End(message);
        }

        private void computeStatistics()
        {
            if (CurrentGame != null) CurrentGame.ComputeStats();
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
