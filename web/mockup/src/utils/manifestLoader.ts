import { Manifest, manifestSchema } from "../types/Manifest";

export class ManifestValidationError extends Error {
    constructor(message: string) {
        super(message);
        this.name = "ManifestValidationError";
    }
}

export const validateManifest = (json: unknown): Manifest => {
    const result = manifestSchema.safeParse(json);
    if (!result.success) {
        const errorMessages = result.error.errors.map((e) => `${e.path.join(".")}: ${e.message}`).join(", ");
        throw new ManifestValidationError(`Invalid manifest format: ${errorMessages}`);
    }
    return result.data;
};

export const loadManifestFromFile = (file: File): Promise<Manifest> => {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = (event) => {
            try {
                const json = JSON.parse(event.target?.result as string);
                const manifest = validateManifest(json);
                resolve(manifest);
            } catch (error) {
                if (error instanceof SyntaxError) {
                    reject(new ManifestValidationError("Invalid JSON syntax"));
                } else {
                    reject(error);
                }
            }
        };
        reader.onerror = () => reject(new Error("Failed to read file"));
        reader.readAsText(file);
    });
};
