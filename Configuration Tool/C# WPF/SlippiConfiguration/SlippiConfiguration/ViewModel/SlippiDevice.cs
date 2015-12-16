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

namespace Fizzi.Applications.SlippiConfiguration.ViewModel
{
    class SlippiDevice : INotifyPropertyChanged
    {
        public PhysicalAddress Mac { get; private set; }
        public IPAddress Ip { get; private set; }

        private StringBuilder logBuilder = new StringBuilder();
        public string Log { get { return logBuilder.ToString(); } }

        public SlippiDevice(string mac, IPAddress address)
        {
            Mac = PhysicalAddress.Parse(mac);
            Ip = address;
        }

        public void AppendLog(string message)
        {
            logBuilder.Append(message);
            this.Raise("Log", PropertyChanged);
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
