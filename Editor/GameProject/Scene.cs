using Editor.Components;
using Editor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace Editor.GameProject
{
	[DataContract]
    class Scene : ViewModelBase
    {
		private string _Name;
		[DataMember]
        public string Name
		{
			get => _Name;
			set
			{
				if (_Name != value)
				{
					_Name = value;
					OnPropertyChanged(nameof(Name));
				}
			}
		}

		[DataMember]
        public Project Project { get; private set; }

		private bool _IsActive;
        [DataMember]
		public bool IsActive
		{
			get => _IsActive;
			set
			{
				if (_IsActive != value)
				{
					_IsActive = value;
					OnPropertyChanged(nameof(IsActive));
				}
			}
		}

        [DataMember(Name = nameof(GameEntities))]
		private readonly ObservableCollection<GameEntity> _gameEntities = new();
		public ReadOnlyObservableCollection<GameEntity> GameEntities { get; private set; }

		public ICommand AddGameEntityCommand { get; private set; }
		public ICommand RemoveGameEntityCommand { get; private set; }

		private void AddGameEntity(GameEntity entity, int idx = -1)
		{
			Debug.Assert(!_gameEntities.Contains(entity));
			entity.IsActive = IsActive;
			if (idx == -1) _gameEntities.Add(entity);
			else _gameEntities.Insert(idx, entity);
		}

		private void RemoveGameEntity(GameEntity entity)
		{
			Debug.Assert(_gameEntities.Contains(entity));
			entity.IsActive = false;
			_gameEntities.Remove(entity);
		}

		[OnDeserialized]
		private void OnDeserialized(StreamingContext context)
		{
			if (_gameEntities != null)
			{
				GameEntities = new ReadOnlyObservableCollection<GameEntity>(_gameEntities);
				OnPropertyChanged(nameof(GameEntities));
			}

			foreach (var entity in _gameEntities) entity.IsActive = IsActive;
			
			AddGameEntityCommand = new CommandRelay<GameEntity>(x =>
			{
				AddGameEntity(x);
				var entityIdx = _gameEntities.Count - 1;

				Project.UndoRedo.Add(new UndoRedoAction(
					() => RemoveGameEntity(x),
					() => AddGameEntity(x, entityIdx),
					$"Add {x.Name} to {Name}"));
			});

			RemoveGameEntityCommand = new CommandRelay<GameEntity>(x =>
			{
				var entityIdx = _gameEntities.IndexOf(x);
				RemoveGameEntity(x);

				Project.UndoRedo.Add(new UndoRedoAction(
					()=> AddGameEntity(x, entityIdx),
					()=> RemoveGameEntity(x),
					$"Remove {x.Name} from {Name}"));
			});
		}

        public Scene(Project project, string name)
        {
			Debug.Assert(project != null);
			Project = project;
			Name = name;
			OnDeserialized(new StreamingContext());
        }
    }
}
