using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class Player
    {
        //Static Data
        public ExternalCharacter CharacterId { get; set; }
        public byte CharacterColor { get; set; }
        public byte PlayerType { get; set; }
        public byte ControllerPort { get; set; }

        //Update Data
        public PlayerFrameData CurrentFrameData { get; set; }

        //Used for Statistic Calculation
        public PlayerFrameData PreviousFrameData { get; set; }
        public PlayerStatistics Stats { get; set; }
        public PlayerFlags Flags { get; set; }

        public Player()
        {
            CurrentFrameData = new PlayerFrameData();
            PreviousFrameData = new PlayerFrameData();
            Stats = new PlayerStatistics();
            Flags = new PlayerFlags();
        }
    }
}
