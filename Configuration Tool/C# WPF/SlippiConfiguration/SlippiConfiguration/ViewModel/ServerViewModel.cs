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
using System.Windows.Data;

namespace Fizzi.Applications.SlippiConfiguration.ViewModel
{
    class ServerViewModel : INotifyPropertyChanged
    {
        private TcpListener listener = null;

        public ObservableCollection<SlippiConnection> Connections { get; private set; }
        private object syncLock = new object();

        private SlippiConnection _selectedConnection;
        public SlippiConnection SelectedConnection { get { return _selectedConnection; } set { this.RaiseAndSetIfChanged("SelectedConnection", ref _selectedConnection, value, PropertyChanged); } }

        public ServerViewModel()
        {
            Connections = new ObservableCollection<SlippiConnection>();

            //Enable updating collection from background thread
            BindingOperations.EnableCollectionSynchronization(Connections, syncLock);
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
                Observable.Start(() =>
                {
                    while(Settings.Default.ServerEnabled)
                    {
                        //Accept a client connection - blocking call
                        var client = listener.AcceptTcpClient();
                        var stream = client.GetStream();

                        //Create a slippi connection object
                        var connection = new SlippiConnection(client, stream);

                        //Add slippi connection to collection, prep it to be removed if an error is encountered
                        lock(syncLock) Connections.Add(connection);
                        connection.TerminateAction = () =>
                        {
                            lock (syncLock) Connections.Remove(connection);
                        };
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
                lock (syncLock) Connections.Clear();
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
