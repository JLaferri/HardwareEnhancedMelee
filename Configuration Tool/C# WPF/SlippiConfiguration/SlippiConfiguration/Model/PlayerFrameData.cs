using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Fizzi.Applications.SlippiConfiguration.Common;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class PlayerFrameData : INotifyPropertyChanged
    {
        //Set up backing fields for the fields that we want to appear on the UI
        private UInt16 _animation;
        private byte _stocks;
        private float _percent;

        //Volatile data - this data will change throughout the course of the match
        public byte InternalCharacterId { get; set; }
        public UInt16 Animation { get { return _animation; } set { this.RaiseAndSetIfChanged("Animation", ref _animation, value, PropertyChanged); } }
        public float LocationX { get; set; }
        public float LocationY { get; set; }
        public byte Stocks { get { return _stocks; } set { this.RaiseAndSetIfChanged("Stocks", ref _stocks, value, PropertyChanged); } }
        public float Percent { get { return _percent; } set { this.RaiseAndSetIfChanged("Percent", ref _percent, value, PropertyChanged); } }
        public float ShieldSize { get; set; }
        public byte LastMoveHitId { get; set; }
        public byte ComboCount { get; set; }
        public byte LastHitBy { get; set; }

        //Controller information
        public float JoystickX { get; set; }
        public float JoystickY { get; set; }
        public float CstickX { get; set; }
        public float CstickY { get; set; }
        public float Trigger { get; set; }
        public UInt32 Buttons { get; set; } //This will include multiple "buttons" pressed on special buttons. For example I think pressing z sets 3 bits

        //This is extra controller information
        public UInt16 PhysicalButtons { get; set; } //A better representation of what a player is actually pressing
        public float LTrigger { get; set; }
        public float RTrigger { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
