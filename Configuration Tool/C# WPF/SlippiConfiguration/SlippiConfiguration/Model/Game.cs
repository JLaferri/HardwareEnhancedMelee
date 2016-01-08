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

            //TODO: Store game information somewhere for calculation/replay purposes
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

                //------------------- Compute flags used for various stats --------------------------------
                var lostStock = cp.PreviousFrameData.Stocks - cp.CurrentFrameData.Stocks > 0;
                var opntLostStock = op.PreviousFrameData.Stocks - op.CurrentFrameData.Stocks > 0;

                //------------------- Increment Action Count for APM Calculation --------------------------------
                //First count the number of buttons that go from 0 to 1
                var buttonChanges = (~cp.PreviousFrameData.PhysicalButtons & cp.CurrentFrameData.PhysicalButtons) & 0xFFF;
                cp.Stats.ActionCount += (uint)numberOfSetBits(buttonChanges); //Increment action count by amount of button presses

                //Increment action count when sticks change from one region to another. Don't increment when stick returns to deadzone
                var prevAnalogRegion = GameStateHelpers.GetJoystickRegion(cp.PreviousFrameData.JoystickX, cp.PreviousFrameData.JoystickY);
                var currentAnalogRegion = GameStateHelpers.GetJoystickRegion(cp.CurrentFrameData.JoystickX, cp.CurrentFrameData.JoystickY);
                if ((prevAnalogRegion != currentAnalogRegion) && (currentAnalogRegion != 0)) cp.Stats.ActionCount++;

                //Do the same for c-stick
                var prevCstickRegion = GameStateHelpers.GetJoystickRegion(cp.PreviousFrameData.CstickX, cp.PreviousFrameData.CstickY);
                var currentCstickRegion = GameStateHelpers.GetJoystickRegion(cp.CurrentFrameData.CstickX, cp.CurrentFrameData.CstickY);
                if ((prevCstickRegion != currentCstickRegion) && (currentCstickRegion != 0)) cp.Stats.ActionCount++;

                //Increment action on analog trigger... I'm not sure when. This needs revision
                if (cp.PreviousFrameData.LTrigger < 0.3 && cp.CurrentFrameData.LTrigger >= 0.3) cp.Stats.ActionCount++;
                if (cp.PreviousFrameData.RTrigger < 0.3 && cp.CurrentFrameData.RTrigger >= 0.3) cp.Stats.ActionCount++;

                ////------------------------------- Monitor Combo Strings -----------------------------------------
                //bool opntTookDamage = op.CurrentFrameData.Percent - op.PreviousFrameData.Percent > 0;
                //bool opntDamagedState = op.CurrentFrameData.Animation >= DAMAGE_START && op.CurrentFrameData.Animation <= DAMAGE_END;
                //bool opntGrabbedState = op.CurrentFrameData.Animation >= CAPTURE_START && op.CurrentFrameData.Animation <= CAPTURE_END;
                //bool opntTechState = (op.CurrentFrameData.Animation >= TECH_START && op.CurrentFrameData.Animation <= TECH_END) ||
                //  op.CurrentFrameData.Animation == TECH_MISS_UP || op.CurrentFrameData.Animation == TECH_MISS_DOWN;

                ////By looking for percent changes we can increment counter even when a player gets true combo'd
                ////The damage state requirement makes it so things like fox's lasers, grab pummels, pichu damaging self, etc don't increment count
                //if (opntTookDamage && (opntDamagedState || opntGrabbedState))
                //{
                //    if (cp.Flags.stringCount == 0)
                //    {
                //        cp.Flags.stringStartPercent = op.PreviousFrameData.Percent;
                //        cp.Flags.stringStartFrame = FrameCounter;
                //        cp.Stats.numberOfOpenings++;
                //        //Serial.print(String("Player ") + (char)(65 + i)); Serial.println(" got an opening!");
                //    }

                //    cp.Flags.stringCount++; //increment number of hits
                //}

                ////Reset combo string counter when somebody dies or doesn't get hit for too long
                //if (opntDamagedState || opntGrabbedState || opntTechState) cp.Flags.stringResetCounter = 0;
                //else if (cp.Flags.stringCount > 0) cp.Flags.stringResetCounter++;

                ////Mark combo completed if opponent lost his stock or if the counter is greater than threshold frames
                //if (cp.Flags.stringCount > 0 && (opntLostStock || lostStock || cp.Flags.stringResetCounter > COMBO_STRING_TIMEOUT))
                //{
                //    //Store records
                //    float percent = op.PreviousFrameData.Percent - cp.Flags.stringStartPercent;
                //    uint32_t frames = frameCounter - cp.Flags.stringStartFrame;
                //    uint16_t hits = cp.Flags.stringCount;

                //    cp.Stats.averageDamagePerString = ((cp.Stats.numberOfOpenings - 1) * cp.Stats.averageDamagePerString + percent) / cp.Stats.numberOfOpenings;
                //    cp.Stats.averageTimePerString = ((cp.Stats.numberOfOpenings - 1) * cp.Stats.averageTimePerString + frames) / cp.Stats.numberOfOpenings;
                //    cp.Stats.averageHitsPerString = ((cp.Stats.numberOfOpenings - 1) * cp.Stats.averageHitsPerString + hits) / cp.Stats.numberOfOpenings;

                //    if (percent > cp.Stats.mostDamageString) cp.Stats.mostDamageString = percent;
                //    if (frames > cp.Stats.mostTimeString) cp.Stats.mostTimeString = frames;
                //    if (hits > cp.Stats.mostHitsString) cp.Stats.mostHitsString = hits;

                //    //Serial.print(String("Player ") + (char)(65 + i)); Serial.println(" combo ended.");

                //    //Reset string count
                //    cp.Flags.stringCount = 0;
                //}

                ////--------------------------- Recovery detection --------------------------------------------------
                //bool isOffStage = GameStateHelpers.CheckIfOffStage(Stage, cp.CurrentFrameData.LocationX, cp.CurrentFrameData.LocationY);
                //bool isInControl = cp.CurrentFrameData.Animation >= GROUNDED_CONTROL_START && cp.CurrentFrameData.Animation <= GROUNDED_CONTROL_END;
                //bool beingDamaged = cp.CurrentFrameData.Animation >= DAMAGE_START && cp.CurrentFrameData.Animation <= DAMAGE_END;
                //bool beingGrabbed = cp.CurrentFrameData.Animation >= CAPTURE_START && cp.CurrentFrameData.Animation <= CAPTURE_END;

                //if (!cp.Flags.IsRecovering && !cp.Flags.IsHitOffStage && beingDamaged && isOffStage)
                //{
                //    //If player took a hit off stage
                //    cp.Flags.IsHitOffStage = true;
                //    //Serial.print(String("Player ") + (char)(65 + i)); Serial.println(String(" off stage! (") + cp.CurrentFrameData.LocationX + String(",") + cp.CurrentFrameData.LocationY + String(")"));
                //}
                //else if (!cp.Flags.IsRecovering && cp.Flags.IsHitOffStage && !beingDamaged && isOffStage)
                //{
                //    //If player exited damage state off stage
                //    cp.Flags.IsRecovering = true;
                //    //Serial.print(String("Player ") + (char)(65 + i)); Serial.println(" recovering!");
                //}
                //else if (!cp.Flags.IsLandedOnStage && (cp.Flags.IsRecovering || cp.Flags.IsHitOffStage) && isInControl)
                //{
                //    //If a player is in control of his character after recovering flag as landed
                //    cp.Flags.IsLandedOnStage = true;
                //    //Serial.print(String("Player ") + (char)(65 + i)); Serial.println(" landed!");
                //}
                //else if (cp.Flags.IsLandedOnStage && isOffStage)
                //{
                //    //If player landed but is sent back off stage, continue recovery process
                //    cp.Flags.FramesSinceLanding = 0;
                //    cp.Flags.IsLandedOnStage = false;
                //}
                //else if (cp.Flags.IsLandedOnStage && !isOffStage && !beingDamaged && !beingGrabbed)
                //{
                //    //If player landed, is still on stage, is not being hit, and is not grabbed, increment frame counter
                //    cp.Flags.FramesSinceLanding++;

                //    //If frame counter while on stage passes threshold, consider it a successful recovery
                //    if (cp.Flags.FramesSinceLanding > FRAMES_LANDED_RECOVERY)
                //    {
                //        if (cp.Flags.IsRecovering)
                //        {
                //            cp.Stats.RecoveryAttempts++;
                //            cp.Stats.SuccessfulRecoveries++;
                //            op.Stats.EdgeguardChances++;
                //            //Serial.print(String("Player ") + (char)(65 + i)); Serial.println(" recovered!");
                //        }

                //        cp.Flags.ResetRecoveryFlags();
                //    }
                //}
                //else if (cp.Flags.IsRecovering && lostStock)
                //{
                //    //If player dies while recovering, consider it a failed recovery
                //    if (cp.Flags.IsRecovering)
                //    {
                //        cp.Stats.recoveryAttempts++;
                //        op.Stats.edgeguardChances++;
                //        op.Stats.edgeguardConversions++;
                //        //Serial.print(String("Player ") + (char)(65 + i)); Serial.println(" died recovering!");
                //    }

                //    cp.Flags.ResetRecoveryFlags();
                //}
            }
        }
    }
}
