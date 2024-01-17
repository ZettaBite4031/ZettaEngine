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
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace Editor.GameProject
{
    /// <summary>
    /// Interaction logic for ProjectBrowserDialog.xaml
    /// </summary>
    public partial class ProjectBrowserDialog : Window
    {
        private readonly CubicEase _easing = new() { EasingMode = EasingMode.EaseInOut };

        public static bool GoToNewProjectTab { get; set; }

        public ProjectBrowserDialog()
        {
            InitializeComponent();
            Loaded += OnProjectBrowserDialogLoaded;
        }

        private void OnProjectBrowserDialogLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnProjectBrowserDialogLoaded;
            if (!OpenProject.Projects.Any() || GoToNewProjectTab)
            {
                if (!GoToNewProjectTab)
                {
                    openProjectbutton.IsEnabled = false;
                    openProjectView.Visibility = Visibility.Hidden;
                }
                OnToggleButton_Click(newProjectbutton, new RoutedEventArgs());
            }
            GoToNewProjectTab = false;
        }

        private void AnimateToNewProject()
        {
            var highlightAnimation = new DoubleAnimation(240, 420, new Duration(TimeSpan.FromSeconds(0.2)));
            highlightAnimation.Completed += (s, e) =>
            {
                var animation = new ThicknessAnimation(new Thickness(0), new Thickness(-1600, 0, 0, 0), new Duration(TimeSpan.FromSeconds(0.5)))
                {
                    EasingFunction = _easing
                };
                browserContent.BeginAnimation(MarginProperty, animation);
            };
            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnimation);
        }

        private void AnimateToOpenProject()
        {
            var highlightAnimation = new DoubleAnimation(420, 240, new Duration(TimeSpan.FromSeconds(0.2)));
            highlightAnimation.Completed += (s, e) =>
            {
                var animation = new ThicknessAnimation(new Thickness(-1600, 0, 0, 0), new Thickness(0), new Duration(TimeSpan.FromSeconds(0.5)))
                {
                    EasingFunction = _easing
                };
                browserContent.BeginAnimation(MarginProperty, animation);
            };
            highlightRect.BeginAnimation(Canvas.LeftProperty, highlightAnimation);
        }

        private void OnToggleButton_Click(object sender, RoutedEventArgs e)
        {
            if (sender == openProjectbutton)
            {
                if (newProjectbutton.IsChecked == true)
                {
                    newProjectbutton.IsChecked = false;
                    AnimateToOpenProject();
                    openProjectView.IsEnabled = true;
                    newProjectView.IsEnabled = false;
                }
                openProjectbutton.IsChecked = true;
            }
            else
            {
                if (openProjectbutton.IsChecked == true)
                {
                    openProjectbutton.IsChecked = false;
                    AnimateToNewProject();
                    openProjectView.IsEnabled = false;
                    newProjectView.IsEnabled = true;
                }
                newProjectbutton.IsChecked = true;
            }
        }
    }
}
