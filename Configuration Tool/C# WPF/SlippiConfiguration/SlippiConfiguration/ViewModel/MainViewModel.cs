using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using Fizzi.Applications.SlippiConfiguration.Common;
using System.ComponentModel;
using System.Windows.Input;
using System.Windows;
using System.Net.Sockets;
using System.Net;
using System.Reactive.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NLog;
using Fizzi.Applications.SlippiConfiguration.Model;

namespace Fizzi.Applications.SlippiConfiguration.ViewModel
{
    class MainViewModel : INotifyPropertyChanged
    {
        public string Version { get { return Assembly.GetExecutingAssembly().GetName().Version.ToString(); } }

        private static Logger logger = LogManager.GetCurrentClassLogger();

        private bool _isBusy;
        public bool IsBusy { get { return _isBusy; } set { this.RaiseAndSetIfChanged("IsBusy", ref _isBusy, value, PropertyChanged); } }

        private Dictionary<IPAddress, SlippiDevice> _devices;
        public Dictionary<IPAddress, SlippiDevice> Devices { get { return _devices; } set { this.RaiseAndSetIfChanged("Devices", ref _devices, value, PropertyChanged); } }

        public SlippiDevice _selectedDevice;
        public SlippiDevice SelectedDevice { get { return _selectedDevice; } set { this.RaiseAndSetIfChanged("SelectedDevice", ref _selectedDevice, value, PropertyChanged); } }

        public DateTime LastScanTime { get; private set; }

        public ICommand ScanDevices { get; private set; }

        public MainViewModel(Window window)
        {
            //Set up procedure for scanning for available devices
            ScanDevices = Command.CreateAsync(() => true, () =>
            {
                //Clear out current list of devices
                Devices = new Dictionary<IPAddress,SlippiDevice>(); 

                UdpClient client = new UdpClient();
                IPEndPoint ip = new IPEndPoint(IPAddress.Broadcast, 3637);
                byte[] bytes = Encoding.ASCII.GetBytes(string.Format("{{\"type\":{0}}}", (int)MessageType.Discovery));
                client.Send(bytes, bytes.Length, ip);
                
                //Log current time
                LastScanTime = DateTime.UtcNow;

                //Prepare to add devices
                var newDevices = new Dictionary<IPAddress, SlippiDevice>();
                
                //Receive responses for one second
                while (DateTime.UtcNow - LastScanTime < TimeSpan.FromSeconds(5))
                {
                    if (client.Available > 0)
                    {
                        //Receive UDP messages
                        IPEndPoint endpoint = null;
                        var response = client.Receive(ref endpoint);
                        
                        //If response is too long, skip
                        if (response.Length > 100) continue;

                        //Parse JSON
                        var json = JObject.Parse(Encoding.ASCII.GetString(response));
                        var command = json["type"].Value<MessageType>();
                        if (command != MessageType.Discovery) continue;

                        //Add device
                        var macString = json["mac"].Value<string>();
                        newDevices[endpoint.Address] = new SlippiDevice(macString, endpoint.Address);
                    }
                }

                //Display new devices
                Devices = newDevices.OrderBy(kvp => kvp.Key.ToString()).ToDictionary(kvp => kvp.Key, kvp => kvp.Value);

                var scanTime = LastScanTime;

                Observable.Start(() =>
                {
                    //Receive responses for one second
                    while (scanTime == LastScanTime)
                    {
                        if (client.Available > 0)
                        {
                            try
                            {
                                //Receive UDP message
                                IPEndPoint endpoint = null;
                                var response = client.Receive(ref endpoint);

                                //Check if the UDP message received comes from a Slippi Device we detected during our last scan
                                SlippiDevice device;
                                if (Devices.TryGetValue(endpoint.Address, out device))
                                {
                                    device.HandleUdpMessage(response);
                                }
                            }
                            catch(Exception ex)
                            {
                                logger.Error(ex, "Error encountered listening for debug messages.");
                            }
                        }
                    }
                }, System.Reactive.Concurrency.TaskPoolScheduler.Default);
            }, () => IsBusy = true, () => IsBusy = false, ex =>
            {
                logger.Error(ex, "Error encountered executing scan for devices.");
                MessageBox.Show(window, ex.NewLineDelimitedMessages(), "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                IsBusy = false;
            });
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
