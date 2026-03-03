
export interface FirmwareActionDefinition {
  id: string;
  name: string;
  description?: string;
  triggers?: string[];
  // validations?: ... ?
}

export interface FirmwareValueDefinition {
  id: string;
  name: string;
  type: "int" | "float" | "string" | "bool";
  readOnly: boolean;
}

export interface FirmwareActionManifest {
  actions: FirmwareActionDefinition[];
  values: FirmwareValueDefinition[];
}

