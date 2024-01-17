using Editor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.IO.Packaging;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Editor.GameProject
{
    [DataContract]
    public class ProjectTemplate
    {
        [DataMember]
        public string ProjectType { get; set; }
        [DataMember]
        public string ProjectFile { get; set; }
        [DataMember]
        public List<string> Folders { get; set; }
        
        public byte[] Icon { get; set; }
        public byte[] Screenshot { get; set; }
        public string IconFilePath { get; set; }
        public string ScreenshotFilePath { get; set; }
        public string ProjectFilePath { get; set; }
        public string TemplatePath { get; set; }
    }

    public class NewProject : ViewModelBase
    {
        // TODO: get path from install location
        private readonly string _templatePath = @"..\..\Editor\ProjectTemplates";

        private string _ProjectName = "NewProject";
        public string ProjectName
        {
            get => _ProjectName;
            set
            {
                if (_ProjectName != value)
                {
                    _ProjectName = value;
                    ValidateProjectPath();
                    OnPropertyChanged(nameof(ProjectName));
                }
            }
        }

        private string _ProjectPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\ZettaProjects\";
        public string ProjectPath
        {
            get => _ProjectPath;
            set
            {
                if (_ProjectPath != value)
                {
                    _ProjectPath = value;
                    ValidateProjectPath();
                    OnPropertyChanged(nameof(ProjectPath));
                }
            }
        }

        private bool _IsValid;
        public bool IsValid
        {
            get => _IsValid;
            set
            {
                if (_IsValid != value)
                {
                    _IsValid = value;
                    OnPropertyChanged(nameof(IsValid));
                }
            }
        }

        private string _ErrorMsg;
        public string ErrorMsg
        {
            get => _ErrorMsg;
            set
            {
                if (_ErrorMsg != value)
                {
                    _ErrorMsg = value;
                    OnPropertyChanged(nameof(ErrorMsg));
                }
            }
        }

        private readonly ObservableCollection<ProjectTemplate> _projectTemplates = new();
        public ReadOnlyObservableCollection<ProjectTemplate> ProjectTemplates { get; }

        private bool ValidateProjectPath()
        {
            var path = ProjectPath;
            if (!Path.EndsInDirectorySeparator(path)) path += @"\";
            path += $@"{ProjectName}\";
            var nameRegex = new Regex(@"^[A-Za-z_][A-Za-z0-9_]*$");

            IsValid = false;
            if (string.IsNullOrWhiteSpace(ProjectName.Trim()))
            {
                ErrorMsg = "Type in a project name.";
            }
            else if (!nameRegex.IsMatch(ProjectName))
            {
                ErrorMsg = "Invalid character(s) in project name";
            }
            else if (string.IsNullOrWhiteSpace(ProjectPath.Trim()))
            {
                ErrorMsg = "Select a valid project folder.";
            }
            else if (ProjectPath.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                char invalid_char = path[ProjectPath.IndexOfAny(Path.GetInvalidPathChars())];
                ErrorMsg = $"Invalid character in project path: {invalid_char}";
            }
            else if (Directory.Exists(path) && Directory.EnumerateFileSystemEntries(path).Any())
            {
                ErrorMsg = "Selected project folder already exists and is not empty.";
            }
            else
            {
                ErrorMsg = string.Empty;
                IsValid = true;
            }

            return IsValid;
        }

        public string CreateProject(ProjectTemplate template)
        {
            ValidateProjectPath();
            if (!IsValid) return string.Empty;

            if (!Path.EndsInDirectorySeparator(ProjectPath)) ProjectPath += @"\";
            var path = $@"{ProjectPath}{ProjectName}\";

            try
            {
                if (!Directory.Exists(path)) Directory.CreateDirectory(path);
                foreach (var folder in template.Folders)
                    Directory.CreateDirectory(Path.GetFullPath(Path.Combine(Path.GetDirectoryName(path), folder)));
                var dirInfo = new DirectoryInfo(path + @".zetta\");
                dirInfo.Attributes |= FileAttributes.Hidden;
                File.Copy(template.IconFilePath, Path.GetFullPath(Path.Combine(dirInfo.FullName, "icon.png")));
                File.Copy(template.ScreenshotFilePath, Path.GetFullPath(Path.Combine(dirInfo.FullName, "screenshot.png")));

                var projectXML = File.ReadAllText(template.ProjectFilePath);
                projectXML = string.Format(projectXML, ProjectName, ProjectPath);
                var projectPath = Path.GetFullPath(Path.Combine(path, $"{ProjectName}{Project.Extension}"));   
                File.WriteAllText(projectPath, projectXML);

                CreateMSVCSolution(template, path);

                return path;
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to create {ProjectName}");
                throw;
            }
        }

        private void CreateMSVCSolution(ProjectTemplate template, string path)
        {
            Debug.Assert(File.Exists(Path.Combine(template.TemplatePath, "MSVCSolution")));
            Debug.Assert(File.Exists(Path.Combine(template.TemplatePath, "MSVCProject")));

            var engineAPIPath = @"$(ZETTA_ENGINE)Engine\EngineAPI\";
            Debug.Assert(Directory.Exists(engineAPIPath));

            var _0 = ProjectName;
            var _1 = "{" + Guid.NewGuid().ToString().ToUpper() + "}";
            var _2 = engineAPIPath;
            var _3 = "$(ZETTA_ENGINE)";

            var solution = File.ReadAllText(Path.Combine(template.TemplatePath, "MSVCSolution"));
            solution = string.Format(solution, _0, _1, "{" + Guid.NewGuid().ToString().ToUpper() + "}");
            File.WriteAllText(Path.GetFullPath(Path.Combine(path, $"{_0}.sln")), solution);

            var project = File.ReadAllText(Path.Combine(template.TemplatePath, "MSVCProject"));
            project = string.Format(project, _0, _1, _2, _3);
            File.WriteAllText(Path.GetFullPath(Path.Combine(path, @$"GameCode\{_0}.vcxproj")), project);
        }

        public NewProject()
        {
            ProjectTemplates = new ReadOnlyObservableCollection<ProjectTemplate>(_projectTemplates);
            try
            {
                var templates = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);
                Debug.Assert(templates.Any());
                foreach (var template in templates)
                {
                    var _template = Serializer.FromFile<ProjectTemplate>(template);
                    _template.IconFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(template), "icon.png"));
                    _template.ScreenshotFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(template), "screenshot.png"));
                    _template.Icon = File.ReadAllBytes(_template.IconFilePath);
                    _template.Screenshot = File.ReadAllBytes(_template.ScreenshotFilePath);
                    _template.ProjectFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(template), _template.ProjectFile));
                    _template.TemplatePath = Path.GetDirectoryName(template);

                    _projectTemplates.Add(_template);
                }
                ValidateProjectPath();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to read project templates");
                throw;
            }
        }
    }
}
