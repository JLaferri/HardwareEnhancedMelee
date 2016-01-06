using Fizzi.Applications.SlippiConfiguration.Properties;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Net.Sockets;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using Fizzi.Applications.SlippiConfiguration.Common;
using System.Windows.Threading;

namespace Fizzi.Applications.SlippiConfiguration.ViewModel
{
    class ServerViewModel : INotifyPropertyChanged
    {
        private TcpListener listener = null;

        public ObservableCollection<SlippiConnection> Connections { get; private set; }

        private SlippiConnection _selectedConnection;
        public SlippiConnection SelectedConnection { get { return _selectedConnection; } set { this.RaiseAndSetIfChanged("SelectedConnection", ref _selectedConnection, value, PropertyChanged); } }

        public ServerViewModel()
        {
            Connections = new ObservableCollection<SlippiConnection>();
            Connections.Add(new SlippiConnection("192.168.1.1"));
        }

        public void StartServer()
        {
            //Don't start server unless it is currently not started
            if (listener == null)
            {
                //Indicate that the server is currently enabled
                Settings.Default.ServerEnabled = true;

                //Start listener on the configured port
                listener = TcpListener.Create(Settings.Default.ServerPort);
                listener.Start();

                var context = System.Threading.Thread.CurrentContext;

                //Listen for Tcp connections until server is disabled
                Observable.Start(async () =>
                {
                    while(Settings.Default.ServerEnabled)
                    {
                        //Accept a client connection - blocking call
                        var client = listener.AcceptTcpClient();
                        var stream = client.GetStream();

                        //Create a slippi connection object
                        var connection = new SlippiConnection(client, stream);

                        //Add slippi connection to collection, prep it to be removed if an error is encountered
                        await Dispatcher.CurrentDispatcher.InvokeAsync(() => Connections.Add(connection));
                        connection.TerminateAction = async () => await Dispatcher.CurrentDispatcher.InvokeAsync(() => Connections.Remove(connection));
                    }
                }, System.Reactive.Concurrency.TaskPoolScheduler.Default);
            }
        }

        public void StopServer()
        {
            //Don't attempt to stop server if it isn't already started
            if (listener != null)
            {
                //Mark server as disabled and stop server if it exists
                Settings.Default.ServerEnabled = false;

                //Stop server
                listener.Stop();
                listener = null;

                //Clear all connections
                Connections.Clear();
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
