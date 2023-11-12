using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.Serialization;
using System.IO;

namespace Editor.Components
{
    [DataContract]
    class Script : Component
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

        public override IMSComponent GetMSComponent(MSEntity msEntity) => new MSScript(msEntity);

        public override void WriteToBinary(BinaryWriter bw)
        {
			var bytes = Encoding.UTF8.GetBytes(Name);
			bw.Write(bytes.Length);
			bw.Write(bytes);
        }

        public Script(GameEntity owner) : base(owner) { }
    }

    sealed class MSScript : MSComponent<Script>
    {

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

        protected override bool UpdateComponents(string propertyName)
        {
            if (propertyName == nameof(Name))
			{
				SelectedComponents.ForEach(c => c.Name = _Name);
				return true;
			}
			return false;
        }

        protected override bool UpdateMSComponent()
        {
			Name = MSEntity.GetMixedValues(SelectedComponents, new Func<Script, string>(x => x.Name));
			return true;
        }

        public MSScript(MSEntity msEntity) : base(msEntity) { Refresh(); }
    }
}
