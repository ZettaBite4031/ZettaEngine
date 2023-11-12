using System;
using System.Collections.Generic;
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

namespace Editor.GameProject
{
    /// <summary>
    /// Interaction logic for OpenProjectView.xaml
    /// </summary>
    public partial class OpenProjectView : UserControl
    {
        public OpenProjectView()
        {
            InitializeComponent();
            Loaded += (s, e) =>
            {
                var item = projectsListBox.ItemContainerGenerator
                .ContainerFromItem(projectsListBox.SelectedItem) as ListBoxItem;
                item?.Focus();
            };
        }

        private void OnOpen_Button_Click(object sender, RoutedEventArgs e)
        {
            OpenSelectedProject();
        }

        private void OnListBoxItem_Mouse_DoubleClick(object sender, MouseButtonEventArgs e)
        {
            OpenSelectedProject();
        }

        private void OpenSelectedProject() 
        {
            var project = OpenProject.Open(projectsListBox.SelectedItem as ProjectData);
            var win = Window.GetWindow(this);
            bool res = (project != null);
            if (res) win.DataContext = project;
            win.DialogResult = res;
            win.Close();
        }
    }
}
