using Editor.DLLWrapper;
using Editor.GameProject;
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

namespace Editor.Components
{
    [DataContract]
    [KnownType(typeof(Transform))]
    [KnownType(typeof(Script))]
    class GameEntity : ViewModelBase
    {

        private int _EntityID = ID.INVALID_ID;
        public int EntityID
        {
            get => _EntityID;
            set
            {
                if (_EntityID != value)
                {
                    _EntityID = value;
                    OnPropertyChanged(nameof(EntityID));
                }
            }
        }

        private bool _IsActive;
        public bool IsActive
        {
            get => _IsActive;
            set
            {
                if (_IsActive != value)
                {
                    _IsActive = value;
                    if (_IsActive)
                    {
                        EntityID = EngineAPI.EntityAPI.CreateGameEntity(this);
                        Debug.Assert(ID.IsValid(_EntityID));
                    }
                    else if (ID.IsValid(EntityID)) {
                        EngineAPI.EntityAPI.RemoveGameEntity(this);
                        EntityID = ID.INVALID_ID;
                    }

                    OnPropertyChanged(nameof(IsActive));
                }
            }
        }

        private bool _IsEnabled = true;
        [DataMember]
        public bool IsEnabled
        {
            get => _IsEnabled;
            set
            {
                if (_IsEnabled != value)
                {
                    _IsEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }

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
        public Scene ParentScene { get; private set; }

        [DataMember(Name = nameof(Components))]
        private readonly ObservableCollection<Component> _components = new ObservableCollection<Component>();
        public ReadOnlyObservableCollection<Component> Components { get; private set; }

        public Component GetComponent(Type type) => Components.FirstOrDefault(x => x.GetType() == type);
        public T GetComponent<T>() where T : Component => GetComponent(typeof(T)) as T;

        public bool AddComponent(Component component)
        {
            Debug.Assert(component != null);
            if(!Components.Any(x=>x.GetType() == component.GetType()))
            {
                IsActive = false;
                _components.Add(component);
                IsActive = true;
                return true;
            }
            Logger.Log(MessageType.Warn, $"Entity {Name} already has a {component.GetType().Name} component");
            return false;
        }

        public void RemoveComponent(Component component)
        {
            Debug.Assert(component != null);
            if (component is Transform) return;

            if (_components.Contains(component))
            {
                IsActive = false;
                _components.Remove(component);
                IsActive = true;
            }
        }

        [OnDeserialized]
        void OnDeserialized(StreamingContext context)
        {
            if (_components != null)
            {
                Components = new ReadOnlyObservableCollection<Component>(_components);
                OnPropertyChanged(nameof(Components));
            }
        }

        public GameEntity(Scene parent)
        {
            Debug.Assert(parent != null);
            ParentScene = parent;
            _components.Add(new Transform(this));
            OnDeserialized(new StreamingContext());
        }
    }

    abstract class MSEntity : ViewModelBase
    {
        private bool _enableUpdates = true;
        private bool? _IsEnabled;
        public bool? IsEnabled
        {
            get => _IsEnabled;
            set
            {
                if (_IsEnabled != value)
                {
                    _IsEnabled = value;
                    OnPropertyChanged(nameof(IsEnabled));
                }
            }
        }

        private string _Name;
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

        private readonly ObservableCollection<IMSComponent> _components = new ObservableCollection<IMSComponent>();
        public ReadOnlyObservableCollection<IMSComponent> Components { get; }

        public T GetMSComponent<T>() where T : IMSComponent
        {
            return (T)Components.FirstOrDefault(x=>x.GetType() == typeof(T));
        }

        public List<GameEntity> SelectedEntitys { get; }

        private void MakeComponentList()
        {
            _components.Clear();
            var first = SelectedEntitys.FirstOrDefault();
            if (first == null) return;

            foreach(var c in first.Components)
            {
                var type = c.GetType();
                if (!SelectedEntitys.Skip(1).Any(e => e.GetComponent(type) == null))
                {
                    Debug.Assert(Components.FirstOrDefault(x => x.GetType() == type) == null);
                    _components.Add(c.GetMSComponent(this));
                }
            }
        }

        public static float? GetMixedValues<T>(List<T> objs, Func<T, float> getProperty)
        {
            var value = getProperty(objs.First());
            return objs.Skip(1).Any(x => getProperty(x) != value) ? null : value;
        }

        public static bool? GetMixedValues<T>(List<T> objs, Func<T, bool> getProperty)
        {
            var value = getProperty(objs.First());
            return objs.Skip(1).Any(x => getProperty(x) != value) ? null : value;
        }

        public static string GetMixedValues<T>(List<T> objs, Func<T, string> getProperty)
        {
            var value = getProperty(objs.First());
            return objs.Skip(1).Any(x => getProperty(x) != value) ? null : value;
        }

        protected virtual bool UpdateGameEntities(string propertyName)
        {
            switch(propertyName)
            {
                case nameof(IsEnabled): SelectedEntitys.ForEach(x => x.IsEnabled = IsEnabled.Value); return true;
                case nameof(Name): SelectedEntitys.ForEach(x => x.Name = Name); return true;
            }
            return false;
        }

        protected virtual void UpdateMSGameEntities()
        {
            IsEnabled = GetMixedValues(SelectedEntitys, new Func<GameEntity, bool>(x => x.IsEnabled));
            Name = GetMixedValues(SelectedEntitys, new Func<GameEntity, string>(x => x.Name));
        }

        public void Refresh()
        {
            _enableUpdates = false;
            UpdateMSGameEntities();
            MakeComponentList();
            _enableUpdates = true;
        }

        public MSEntity(List<GameEntity> entities)
        {
            Debug.Assert(entities?.Any() == true);
            Components = new ReadOnlyObservableCollection<IMSComponent>(_components);
            SelectedEntitys = entities;
            PropertyChanged += (s, e) => { if (_enableUpdates) UpdateGameEntities(e.PropertyName); };
        }
    }

    class MSGameEntity : MSEntity
    {
        public MSGameEntity(List<GameEntity> entities) : base(entities)
        {
            Refresh();
        }
    }
}
