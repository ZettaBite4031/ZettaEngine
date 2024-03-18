using Editor.Content;
using Editor.GameDev;
using Editor.GameProject;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Editor.Editors
{
    /// <summary>
    /// Interaction logic for WorldEditorView.xaml
    /// </summary>
    public partial class WorldEditorView : UserControl
    {
        public WorldEditorView()
        {
            InitializeComponent();
            Loaded += OnWorldEditorViewLoaded;
        }

        private void OnWorldEditorViewLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnWorldEditorViewLoaded;
            Focus();
        }

        private void OnNewScript_Button_Click(object sender, RoutedEventArgs e)
        {
            new NewScriptDialog().ShowDialog();
        }

        private void OnCreatePrimtiveMesh_Button_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new PrimitiveMeshDialog();
            dlg.ShowDialog();
        }

        private void UnloadAndCloseAllWindows()
        {
            Project.Current?.Unload();

            var mainWnd = Application.Current.MainWindow;
            foreach (Window win in Application.Current.Windows)
                if (win != mainWnd)
                {
                    win.DataContext = null;
                    win.Close();
                }

            mainWnd.DataContext = null;
            mainWnd.Close();
        }

        private void OnNewProject(object sender, ExecutedRoutedEventArgs e)
        {
            ProjectBrowserDialog.GoToNewProjectTab = true;
            UnloadAndCloseAllWindows();
        }

        private void OnOpenProject(object sender, ExecutedRoutedEventArgs e) => UnloadAndCloseAllWindows();
        

        private void OnEditorClose(object sender, ExecutedRoutedEventArgs e)
        {
            Application.Current.MainWindow.Close();
        }

        private void OnContentBrowser_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if ((sender as FrameworkElement).DataContext is ContentBrowser contentBrowser && string.IsNullOrEmpty(contentBrowser.SelectedFolder?.Trim()))
            {
                contentBrowser.SelectedFolder = contentBrowser.ContentFolder;
            }
        }

        private void OnContentBrowser_Loaded(object sender, RoutedEventArgs e) => OnContentBrowser_IsVisibleChanged(sender, default);
    }
}
