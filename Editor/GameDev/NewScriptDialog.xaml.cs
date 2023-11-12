using Editor.GameProject;
using Editor.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;
using System.Windows.Media.Animation;

namespace Editor.GameDev
{
    /// <summary>
    /// Interaction logic for NewScriptDialog.xaml
    /// </summary>
    public partial class NewScriptDialog : Window
    {
        private static readonly string _sourceCode = @"#include ""{0}.h""

namespace {1} {{
    REGISTER_SCRIPT({0});

    void {0}::OnWake() {{
        // Initialization
    }}

    void {0}::Update(float dt) {{
        // Called every frame
    }}

}} // namespace {1}

";
        private static readonly string _headerCode = @"#pragma once

namespace {1} {{
    
    class {0} : public Zetta::Script::EntityScript {{
    public:
        constexpr explicit {0}(Zetta::GameEntity::Entity entity) 
            : Zetta::Script::EntityScript{{entity}} {{ }}

        void OnWake() override;
        void Update(float) override;
    
    private:
    }};

}} // namespace {1}

";

        public NewScriptDialog()
        {
            InitializeComponent();
            Owner = Application.Current.MainWindow;
            scriptPath.Text = @"GameCode\";
        }

        private static readonly string _namespace = GetNamespaceFromProjectName();

        private static string GetNamespaceFromProjectName()
        {
            var projectName = Project.Current.Name;
            projectName = projectName.Replace(' ', '_');
            return projectName;
        }

        private bool Validate()
        {
            bool isValid = false;
            var name = scriptName.Text.Trim();
            var path = scriptPath.Text.Trim();
            string err = string.Empty;
            if (string.IsNullOrEmpty(name)) err = "Please enter a script name";
            else if (name.IndexOfAny(Path.GetInvalidFileNameChars()) != -1 || name.Any(x => char.IsWhiteSpace(x)))
                err = "Invalid character(s) used in script name";
            else if (string.IsNullOrEmpty(path)) err = "Please enter a script path";
            else if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
                err = "Invalid character(s) used in script path";
            else if (!Path.GetFullPath(Path.Combine(Project.Current.Path, path)).Contains(Path.Combine(Project.Current.Path, @"GameCode\")))
                err = "Script must be added to, or a sub-folder of, GameCode.";
            else if (File.Exists(Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Path, path), $"{name}.cpp"))) ||
                File.Exists(Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Path, path), $"{name}.h"))))
                err = $"Script '{name}' already exists in that folder.";
            else isValid = true;
            if (!isValid) messageTextBlock.Foreground = FindResource("Editor.RedBrush") as Brush;
            else messageTextBlock.Foreground = FindResource("Editor.FontBrush") as Brush;
            messageTextBlock.Text = err;
            return isValid;
        }

        private void OnScriptName_TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (!Validate()) return;
            var name = scriptName.Text.Trim();
            messageTextBlock.Text = $"{name}.h and {name}.cpp will be added to {Project.Current.Name}";
        }

        private void OnScriptPath_TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            Validate();
        }

        private async void OnCreate_Button_Click(object sender, RoutedEventArgs e)
        {
            if (!Validate()) return;
            IsEnabled = false;
            busyAnimation.Opacity = 0;
            busyAnimation.Visibility = Visibility.Visible;
            DoubleAnimation fadeIn = new DoubleAnimation(0, 1, new Duration(TimeSpan.FromMilliseconds(500)));
            busyAnimation.BeginAnimation(OpacityProperty, fadeIn);

            try
            {
                var name = scriptName.Text.Trim();
                var path = Path.GetFullPath(Path.Combine(Path.Combine(Project.Current.Path, Project.Current.Name), scriptPath.Text.Trim()));
                var sln = Project.Current.Solution;
                var project = Project.Current.Name;
                await Task.Run(() => CreateScript(name, path, sln, project));
            } 
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to create script: {scriptName.Text}");
            }
            finally
            {
                DoubleAnimation fadeOut = new DoubleAnimation(1, 0, new Duration(TimeSpan.FromMilliseconds(200)));
                fadeOut.Completed += (s, e) =>
                {
                    busyAnimation.Opacity = 0;
                    busyAnimation.Visibility = Visibility.Hidden;
                    Close();
                };
                busyAnimation.BeginAnimation(OpacityProperty, fadeOut);
            }
        }

        private void CreateScript(string name, string path, string sln, string project)
        {
            if (!Directory.Exists(path)) Directory.CreateDirectory(path);

            var source = Path.GetFullPath(Path.Combine(path, $"{name}.cpp"));
            var header = Path.GetFullPath(Path.Combine(path, $"{name}.h"));

            using(var sw  = File.CreateText(source))
            {
                sw.Write(string.Format(_sourceCode, name, _namespace));
            }
            using (var sw = File.CreateText(header))
            {
                sw.Write(string.Format(_headerCode, name, _namespace));
            }

            string[] files = new string[] { source, header };

            for (int i = 0; i < 3; i++)
            {
                if (!VisualStudio.AddFilesToSolution(sln, project, files)) System.Threading.Thread.Sleep(1000);
                else break;
            }
        }
    }
}
