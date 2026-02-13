import os
import shutil
import joblib
from sklearn.datasets import fetch_openml
from sklearn.model_selection import train_test_split
from sklearn.linear_model import SGDClassifier
from sklearn.preprocessing import StandardScaler
from sklearn.pipeline import make_pipeline
from sklearn.metrics import accuracy_score

DATA_DIR = "/dev/shm/sklearn_data"
MODEL_PATH = "/dev/shm/mnist_sgd_model.joblib"
METRICS_PATH = "/dev/shm/mnist_metrics.txt"
os.environ["SCIKIT_LEARN_DATA"] = DATA_DIR

try:
    X, y = fetch_openml(
        "mnist_784",
        version=1,
        as_frame=False,
        return_X_y=True,
        parser="liac-arff",
    )
    y = y.astype(int)
    X, _, y, _ = train_test_split(X, y, train_size=10_000, random_state=0, stratify=y)
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=0, stratify=y
    )
    clf = make_pipeline(
        StandardScaler(with_mean=False),
        SGDClassifier(loss="log_loss", max_iter=10, tol=1e-3, random_state=0),
    )
    clf.fit(X_train, y_train)
    pred = clf.predict(X_test)
    acc = accuracy_score(y_test, pred)
    print("accuracy:", acc)
    joblib.dump(clf, MODEL_PATH)
    with open(METRICS_PATH, "w") as f:
        f.write(f"accuracy={acc}\n")

finally:
    for p in (MODEL_PATH, METRICS_PATH):
        try:
            os.remove(p)
        except FileNotFoundError:
            pass
    shutil.rmtree(DATA_DIR, ignore_errors=True)
