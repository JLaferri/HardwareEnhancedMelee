using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Fizzi.Applications.SlippiConfiguration.Common;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    class Game
    {
        public Player[] Players { get; set; }
        public UInt16 Stage { get; set; }

        public UInt32 FrameCounter { get; set; }
        public UInt32 FramesMissed { get; set; }
        public UInt32 RandomSeed { get; set; }

        public byte WinCondition { get; set; }

        //Collects all of the bytes used to compute the state of the game and statistics
        public List<byte> GameBytes { get; private set; }

        //Game objects should be created with the New static method
        private Game() { }

        public static Game New(byte[] message)
        {
            var game = new Game();
            game.GameBytes = message.ToList();

            int index = 1;

            //Get stage ID and increment counter
            game.Stage = BitHelper.ReadUInt16(message, ref index);
            
            //Populate players
            game.Players = Enumerable.Range(0, Constants.PLAYER_COUNT).Select(_ =>
            {
                var p = new Player();

                //Load player data
                p.ControllerPort = BitHelper.ReadByte(message, ref index);
                p.CharacterId = BitHelper.ReadByte(message, ref index);
                p.PlayerType = BitHelper.ReadByte(message, ref index);
                p.CharacterColor = BitHelper.ReadByte(message, ref index);

                return p;
            }).ToArray();

            return game;
        }

        public void Update(byte[] message)
        {
            //Add update bytes to GameBytes
            GameBytes.AddRange(message);

            int index = 1;

            //Check frame count and see if any frames were skipped
            var frameCount = BitHelper.ReadUInt32(message, ref index);
            uint framesMissed = frameCount - FrameCounter - 1;
            FramesMissed += framesMissed;
            FrameCounter = frameCount;

            //Load random seed
            RandomSeed = BitHelper.ReadUInt32(message, ref index);

            //Load data for the players
            foreach(var p in Players)
            {
                p.PreviousFrameData = p.CurrentFrameData.Copy(); //Deep copy current frame data into previous. Uses reflection - consider changing if slow
                var pfd = p.CurrentFrameData;

                pfd.InternalCharacterId = BitHelper.ReadByte(message, ref index);
                pfd.Animation = BitHelper.ReadUInt16(message, ref index);
                pfd.LocationX = BitHelper.ReadFloat(message, ref index);
                pfd.LocationY = BitHelper.ReadFloat(message, ref index);

                //Controller information
                pfd.JoystickX = BitHelper.ReadFloat(message, ref index);
                pfd.JoystickY = BitHelper.ReadFloat(message, ref index);
                pfd.CstickX = BitHelper.ReadFloat(message, ref index);
                pfd.CstickY = BitHelper.ReadFloat(message, ref index);
                pfd.Trigger = BitHelper.ReadFloat(message, ref index);
                pfd.Buttons = BitHelper.ReadUInt32(message, ref index);

                //More data
                pfd.Percent = BitHelper.ReadFloat(message, ref index);
                pfd.ShieldSize = BitHelper.ReadFloat(message, ref index);
                pfd.LastMoveHitId = BitHelper.ReadByte(message, ref index);
                pfd.ComboCount = BitHelper.ReadByte(message, ref index);
                pfd.LastHitBy = BitHelper.ReadByte(message, ref index);
                pfd.Stocks = BitHelper.ReadByte(message, ref index);

                //Raw controller information
                pfd.PhysicalButtons = BitHelper.ReadUInt16(message, ref index);
                pfd.LTrigger = BitHelper.ReadFloat(message, ref index);
                pfd.RTrigger = BitHelper.ReadFloat(message, ref index);
            }
        }

        public void End(byte[] message)
        {
            //Add update bytes to GameBytes
            GameBytes.AddRange(message);

            int index = 1;

            //Load win condition
            WinCondition = BitHelper.ReadByte(message, ref index);
        }

        int numberOfSetBits(int x)
        {
            //This function solves the Hamming Weight problem. Effectively it counts the number of bits in the input that are set to 1
            //This implementation is supposedly very efficient when most bits are zero. Found: https://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
            int count;
            for (count = 0; x != 0; count++) x &= x - 1;
            return count;
        }

        public void ComputeStats()
        {
            for (int i = 0; i < Constants.PLAYER_COUNT; i++)
            {
                Player cp = Players[i]; //Current player
                Player op = Players[Math.Abs(i - 1)]; //Other player

                //------------------- Increment Action Count for APM Calculation --------------------------------
                //First count the number of buttons that go from 0 to 1
                var buttonChanges = (~cp.PreviousFrameData.PhysicalButtons & cp.CurrentFrameData.PhysicalButtons) & 0xFFF;
                cp.Stats.ActionCount += (uint)numberOfSetBits(buttonChanges); //Increment action count by amount of button presses

                //Increment action count when sticks change from one region to another. Don't increment when stick returns to deadzone
                var prevAnalogRegion = ControllerHelpers.GetJoystickRegion(cp.PreviousFrameData.JoystickX, cp.PreviousFrameData.JoystickY);
                var currentAnalogRegion = ControllerHelpers.GetJoystickRegion(cp.CurrentFrameData.JoystickX, cp.CurrentFrameData.JoystickY);
                if ((prevAnalogRegion != currentAnalogRegion) && (currentAnalogRegion != 0)) cp.Stats.ActionCount++;

                //Do the same for c-stick
                var prevCstickRegion = ControllerHelpers.GetJoystickRegion(cp.PreviousFrameData.CstickX, cp.PreviousFrameData.CstickY);
                var currentCstickRegion = ControllerHelpers.GetJoystickRegion(cp.CurrentFrameData.CstickX, cp.CurrentFrameData.CstickY);
                if ((prevCstickRegion != currentCstickRegion) && (currentCstickRegion != 0)) cp.Stats.ActionCount++;

                //Increment action on analog trigger... I'm not sure when. This needs revision
                if (cp.PreviousFrameData.LTrigger < 0.3 && cp.CurrentFrameData.LTrigger >= 0.3) cp.Stats.ActionCount++;
                if (cp.PreviousFrameData.RTrigger < 0.3 && cp.CurrentFrameData.RTrigger >= 0.3) cp.Stats.ActionCount++;
            }
        }
    }
}
