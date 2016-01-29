using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading.Tasks;
using Fizzi.Applications.SlippiConfiguration.Common;
using Newtonsoft.Json.Linq;
using Fizzi.Applications.SlippiConfiguration.Model;
using System.Windows.Input;
using System.Net.Sockets;

namespace Fizzi.Applications.SlippiConfiguration.ViewModel
{
    class SlippiDevice : INotifyPropertyChanged
    {
        private const int MAX_LOG_LENGTH = 1000;

        public PhysicalAddress Mac { get; private set; }
        public IPAddress DeviceIp { get; private set; }

        private IPAddress _targetIp;
        public IPAddress TargetIp { get { return _targetIp; } set { this.RaiseAndSetIfChanged("TargetIp", ref _targetIp, value, PropertyChanged); } }

        private int _targetPort;
        public int TargetPort { get { return _targetPort; } set { this.RaiseAndSetIfChanged("TargetPort", ref _targetPort, value, PropertyChanged); } }

        private IPAddress _pendingTargetIp;
        public IPAddress PendingTargetIp { get { return _pendingTargetIp; } set { this.RaiseAndSetIfChanged("PendingTargetIp", ref _pendingTargetIp, value, PropertyChanged); } }

        private int _pendingTargetPort;
        public int PendingTargetPort { get { return _pendingTargetPort; } set { this.RaiseAndSetIfChanged("PendingTargetPort", ref _pendingTargetPort, value, PropertyChanged); } }

        private StringBuilder logBuilder = new StringBuilder(0, MAX_LOG_LENGTH);
        public string Log { get { return logBuilder.ToString(); } }

        public ICommand EraseFlashCommand { get; private set; }
        public ICommand SetTargetCommand { get; private set; }

        public SlippiDevice(string mac, IPAddress address)
        {
            Mac = PhysicalAddress.Parse(mac);
            DeviceIp = address;

            EraseFlashCommand = Command.Create(() => true, EraseFlash);
            SetTargetCommand = Command.Create(() => true, () => ChangeTarget(PendingTargetIp, PendingTargetPort));
        }

        public void HandleUdpMessage(byte[] message)
        {
            //Parse JSON
            var json = JObject.Parse(Encoding.ASCII.GetString(message));
            int command = json["type"].Value<int>();

            switch((UdpMessageType)command)
            {
                case UdpMessageType.LogMessage:
                    //Get string and append to log
                    var logMessage = json["message"].Value<string>();
                    AppendLog(logMessage);
                    break;
            }
            
        }

        public void AppendLog(string message)
        {
            //Don't allow stringbuilder to get bigger than maximum length.
            var newLength = logBuilder.Length + message.Length;
            if (newLength > MAX_LOG_LENGTH)
            {
                //Remove characters from the begining of string builder
                var charsToRemove = newLength - MAX_LOG_LENGTH;
                logBuilder.Remove(0, charsToRemove);
            }

            //Append new message to log string
            logBuilder.Append(message);
            this.Raise("Log", PropertyChanged);
        }

        public void EraseFlash()
        {
            UdpClient client = new UdpClient();
            IPEndPoint ip = new IPEndPoint(DeviceIp, 3637);
            byte[] bytes = Encoding.ASCII.GetBytes(string.Format("{{\"type\":{0}}}", (int)UdpMessageType.FlashErase));
            client.Send(bytes, bytes.Length, ip);
        }

        public void ChangeTarget(IPAddress address, int port)
        {
            UdpClient client = new UdpClient();
            IPEndPoint ip = new IPEndPoint(DeviceIp, 3637);
            var addressBytes = address.GetAddressBytes();
            var sendString = string.Format("{{\"type\":{0},\"ip1\":{1},\"ip2\":{2},\"ip3\":{3},\"ip4\":{4},\"port\":{5}}}",
                (int)UdpMessageType.SetTarget, addressBytes[0], addressBytes[1], addressBytes[2], addressBytes[3], port);
            byte[] bytes = Encoding.ASCII.GetBytes(sendString);
            client.Send(bytes, bytes.Length, ip);
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
