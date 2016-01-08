using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Fizzi.Applications.SlippiConfiguration.Common;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    static class BitHelper
    {
        private static T[] SubArray<T>(this T[] data, int index, int length)
        {
            T[] result = new T[length];
            Array.Copy(data, index, result, 0, length);
            return result;
        }

        public static byte ReadByte(byte[] msg, ref int index)
        {
            var value = msg[index];
            index++;

            return value;
        }

        public static UInt16 ReadUInt16(byte[] msg, ref int index)
        {
            var value = BitConverter.ToUInt16(msg.SubArray(index, 2).Reverse().ToArray(), 0);
            index += 2;

            return value;
        }

        public static UInt32 ReadUInt32(byte[] msg, ref int index)
        {
            var value = BitConverter.ToUInt32(msg.SubArray(index, 4).Reverse().ToArray(), 0);
            index += 4;

            return value;
        }

        public static float ReadFloat(byte[] msg, ref int index)
        {
            var value = BitConverter.ToSingle(msg.SubArray(index, 4).Reverse().ToArray(), 0);
            index += 4;

            return value;
        }
    }
}
