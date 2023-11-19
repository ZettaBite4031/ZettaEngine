using Editor.Content;
using Editor.GameProject;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;

namespace Editor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            Loaded += OnMainWindowLoaded;
            Closing += OnMainWindowClosing;
        }

        public static string ZettaPath { get; private set; } 

        private void GetEnginePath()
        {
            var enginePath = Environment.GetEnvironmentVariable("ZETTA_ENGINE", EnvironmentVariableTarget.User);
            if (enginePath == null || !Directory.Exists(Path.Combine(enginePath, @"Engine\EngineAPI\")) )
            {
                var dlg = new EnginePathDialog();
                if (dlg.ShowDialog() == true)
                {
                    ZettaPath = dlg.ZettaPath;
                    Environment.SetEnvironmentVariable("ZETTA_ENGINE", ZettaPath.ToUpper(), EnvironmentVariableTarget.User);
                }
                else Application.Current.Shutdown();
            }
            else ZettaPath = enginePath;
        }

        private void OnMainWindowLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnMainWindowLoaded;
            GetEnginePath();
            OpenProjectBrowserDialog();
        }

        private void OnMainWindowClosing(object sender, CancelEventArgs e)
        {
            if (DataContext == null)
            {
                e.Cancel = true;
                Application.Current.MainWindow.Hide();
                OpenProjectBrowserDialog();
                if (DataContext != null)
                    Application.Current.MainWindow.Show();
            }
            else
            {
                Closing -= OnMainWindowClosing;
                Project.Current?.Unload();
            }
        }

        private void OpenProjectBrowserDialog()
        {
            var projectBrowser = new ProjectBrowserDialog();
            if (projectBrowser.ShowDialog() == false || projectBrowser.DataContext == null) 
                Application.Current.Shutdown();
            else
            {
                Project.Current?.Unload();
                var project = projectBrowser.DataContext as Project;
                Debug.Assert(project != null);
                ContentWatcher.Reset(project.ContentPath, $@"{project.Path}{project.Name}\");
                DataContext = project;
            }           
        }
    }
}
