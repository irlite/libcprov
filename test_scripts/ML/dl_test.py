import os
import shutil
import torch
import torch.nn as nn
import torch.optim as optim
import torchvision
import torchvision.transforms as transforms

DATA_DIR = "/dev/shm/torch_data"
MODEL_PATH = "/dev/shm/cifar10_tiny_cnn.pt"
METRICS_PATH = "/dev/shm/cifar10_metrics.txt"

torch.set_num_threads(1)

class TinyCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.features = nn.Sequential(
            nn.Conv2d(3, 8, 3, padding=1), nn.ReLU(), nn.MaxPool2d(2),
            nn.Conv2d(8, 16, 3, padding=1), nn.ReLU(), nn.MaxPool2d(2),
        )
        self.classifier = nn.Sequential(
            nn.Flatten(),
            nn.Linear(16 * 8 * 8, 64), nn.ReLU(),
            nn.Linear(64, 10),
        )

    def forward(self, x):
        x = self.features(x)
        return self.classifier(x)

try:
    os.makedirs(DATA_DIR, exist_ok=True)
    transform = transforms.Compose([
        transforms.ToTensor(),
        transforms.Normalize((0.5, 0.5, 0.5), (0.5, 0.5, 0.5)),
    ])
    trainset = torchvision.datasets.CIFAR10(
        root=DATA_DIR, train=True, download=True, transform=transform
    )
    testset = torchvision.datasets.CIFAR10(
        root=DATA_DIR, train=False, download=True, transform=transform
    )
    train_subset = torch.utils.data.Subset(trainset, range(1000))
    test_subset = torch.utils.data.Subset(testset, range(200))
    trainloader = torch.utils.data.DataLoader(
        train_subset, batch_size=32, shuffle=True, num_workers=0
    )
    testloader = torch.utils.data.DataLoader(
        test_subset, batch_size=64, shuffle=False, num_workers=0
    )
    device = torch.device("cpu")
    model = TinyCNN().to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.SGD(model.parameters(), lr=0.02, momentum=0.9)
    model.train()
    for i, (images, labels) in enumerate(trainloader):
        images, labels = images.to(device), labels.to(device)
        optimizer.zero_grad()
        outputs = model(images)
        loss = criterion(outputs, labels)
        loss.backward()
        optimizer.step()
        if i >= 20:
            break
    model.eval()
    correct = 0
    total = 0
    with torch.no_grad():
        for images, labels in testloader:
            images, labels = images.to(device), labels.to(device)
            outputs = model(images)
            preds = outputs.argmax(dim=1)
            total += labels.size(0)
            correct += (preds == labels).sum().item()
    acc = correct / total
    print("accuracy:", acc)
    torch.save(model.state_dict(), MODEL_PATH)
    with open(METRICS_PATH, "w") as f:
        f.write(f"accuracy={acc}\n")

finally:
    for p in (MODEL_PATH, METRICS_PATH):
        try:
            os.remove(p)
        except FileNotFoundError:
            pass
    shutil.rmtree(DATA_DIR, ignore_errors=True)
