using Editor.GameProject;
using System;
using System.Collections.Generic;
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
    /// Interaction logic for Window1.xaml
    /// </summary>
    public partial class EnginePathDialog : Window
    {
        public string ZettaPath { get; set; }

        public EnginePathDialog()
        {
            InitializeComponent();
            Owner = Application.Current.MainWindow;
        }

        private string ValidatePath(string path)
        {
            string _message = string.Empty;
            if (string.IsNullOrWhiteSpace(path.Trim()))
            {
                _message = "Type in a project name.";
            }
            else if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                char invalid_char = path[path.IndexOfAny(Path.GetInvalidPathChars())];
                _message = $"Invalid character in project path: {invalid_char}";
            }
            else if (!Directory.Exists(Path.Combine(path, @"Engine\EngineAPI\")))
            {
                _message = "Unable to find the engine.";
            }

            return _message;
        }

        private void OnOk_Button_Click(object sender, RoutedEventArgs e)
        {
            var path = pathTextBox.Text;
            messageTextBlock.Text = ValidatePath(path);

            if (string.IsNullOrEmpty(messageTextBlock.Text))
            {
                if (!Path.EndsInDirectorySeparator(path)) path += @"\";
                ZettaPath = path;
                DialogResult = true;
                Close();
            }
        }
    }
}
