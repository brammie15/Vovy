#include "Transform.h"

#include <iostream>

Transform::~Transform() {
    SetParent(nullptr);

    for (auto it = m_Children.begin(); it != m_Children.end();) {
        Transform* child = *it;
        it = m_Children.erase(it);
        child->SetParent(nullptr);
    }
}

Transform::Transform(const glm::vec3 position): m_LocalPosition(position),
      m_LocalRotation(glm::quat(1, 0, 0, 0)),
      m_LocalScale(glm::vec3(1.0f)) {
    SetPositionDirty();
    SetRotationDirty();
    SetScaleDirty();
}

void Transform::SetWorldMatrix(const glm::mat4& mat) {
    m_WorldMatrix = mat;

    m_WorldPosition = glm::vec3(mat[3]);

    m_WorldScale = glm::vec3(
        glm::length(glm::vec3(mat[0])),
        glm::length(glm::vec3(mat[1])),
        glm::length(glm::vec3(mat[2]))
    );

    glm::mat3 rotationMatrix = glm::mat3(
        glm::vec3(mat[0]) / m_WorldScale.x,
        glm::vec3(mat[1]) / m_WorldScale.y,
        glm::vec3(mat[2]) / m_WorldScale.z
    );
    m_WorldRotation = glm::quat_cast(rotationMatrix);

    if (m_Parent) {
        glm::mat4 parentInv = glm::inverse(m_Parent->GetWorldMatrix());
        glm::mat4 localMat = parentInv * mat;
        m_LocalPosition = glm::vec3(localMat[3]);
        glm::mat3 localRotationMatrix = glm::mat3(
            glm::vec3(localMat[0]) / m_WorldScale.x,
            glm::vec3(localMat[1]) / m_WorldScale.y,
            glm::vec3(localMat[2]) / m_WorldScale.z
        );
        m_LocalRotation = glm::quat_cast(localRotationMatrix);
        m_LocalScale = m_Parent ? m_WorldScale / m_Parent->GetWorldScale() : m_WorldScale;
    } else {
        m_LocalPosition = m_WorldPosition;
        m_LocalRotation = m_WorldRotation;
        m_LocalScale = m_WorldScale;
    }

    SetPositionDirty();
    SetRotationDirty();
    SetScaleDirty();
}

const glm::vec3& Transform::GetWorldPosition() {
    if (m_PositionDirty) {
        UpdateWorldPosition();
    }
    return m_WorldPosition;
}

const glm::vec3& Transform::GetWorldScale() {
    if (m_ScaleDirty) {
        UpdateWorldScale();
    }
    return m_WorldScale;
}

const glm::quat& Transform::GetWorldRotation() {
    if (m_RotationDirty) {
        UpdateWorldRotation();
    }
    return m_WorldRotation;
}

const glm::mat4& Transform::GetWorldMatrix() {
    if (m_MatrixDirty) {
        UpdateWorldMatrix();
    }
    return m_WorldMatrix;
}

void Transform::SetLocalPosition(const glm::vec3& position) {
    m_LocalPosition = position;
    SetPositionDirty();
}

void Transform::SetLocalPosition(float x, float y, float z) {
    SetLocalPosition(glm::vec3{x, y, z});
}

void Transform::SetLocalRotation(float x, float y, float z) {
    SetLocalRotation({glm::vec3{x, y, z}});
}

void Transform::SetLocalRotation(const glm::vec3& rotation) {
    SetLocalRotation(glm::quat(glm::radians(rotation)));
}

void Transform::SetLocalRotation(const glm::quat& rotation) {
    m_LocalRotation = rotation;
    SetRotationDirty();
}

void Transform::SetLocalScale(float x, float y, float z) {
    SetLocalScale({x, y, z});
}

void Transform::SetLocalScale(const glm::vec3& scale) {
    m_LocalScale = scale;
    SetScaleDirty();
}

void Transform::SetWorldPosition(const glm::vec3& position) {
    if (m_Parent == nullptr) {
        SetLocalPosition(position);
    } else {
        SetLocalPosition(position - m_Parent->GetWorldPosition());
    }
}

void Transform::SetWorldPosition(float x, float y, float z) {
    SetWorldPosition(glm::vec3(x, y, z));
}

void Transform::SetWorldRotation(const glm::quat& rotation) {
    if(m_Parent == nullptr) {
        SetLocalRotation(rotation);
    } else {
        SetLocalRotation(glm::inverse(m_Parent->GetWorldRotation()) * rotation);
    }
}

void Transform::SetWorldRotation(const glm::vec3& rotation) {
    SetWorldRotation(glm::quat(glm::radians(rotation)));
}


void Transform::SetWorldRotation(float x, float y, float z) {
    SetWorldRotation(glm::vec3{x, y, z});
}

void Transform::SetWorldScale(double x, double y, double z) {
    SetWorldScale(glm::vec3{x, y, z});
}

void Transform::SetWorldScale(const glm::vec3& scale) {
    if (m_Parent == nullptr) {
        SetLocalScale(scale);
    } else {
        SetLocalScale(scale / m_Parent->GetWorldScale());
    }
}

void Transform::SetParent(Transform* parent, bool useWorldPosition) {
    if (parent == m_Parent or parent == this or IsChild(parent)) {
        return;
    }

    if (parent == nullptr) {
        SetLocalPosition(GetWorldPosition());
    } else {
        if (useWorldPosition) {
            SetLocalPosition(GetWorldPosition() - parent->GetWorldPosition());
        }
        SetPositionDirty();
    }
    if (m_Parent) {
        m_Parent->RemoveChild(this);
    }
    m_Parent = parent;
    if (m_Parent) {
        m_Parent->AddChild(this);
    }
}

bool Transform::IsChild(Transform* child) const {
    return std::ranges::find(m_Children, child) != m_Children.end();
}

void Transform::AddChild(Transform* transform) {
    m_Children.push_back(transform);

}

void Transform::RemoveChild(Transform* transform) {
    std::erase(m_Children, transform);
}

void Transform::UpdateWorldPosition() {
    if (m_Parent) {
        m_WorldPosition = m_Parent->GetWorldPosition() + m_LocalPosition;
    } else {
        m_WorldPosition = m_LocalPosition;
    }
    m_PositionDirty = false;
}

void Transform::UpdateWorldRotation() {
    if (m_Parent == nullptr) {
        m_WorldRotation = m_LocalRotation;
    } else {
        m_WorldRotation = m_LocalRotation * m_Parent->GetWorldRotation();
    }
    m_RotationDirty = false;
}

void Transform::UpdateWorldScale() {
    if(m_Parent == nullptr) {
        m_WorldScale = m_LocalScale;
    } else {
        m_WorldScale = m_LocalScale * m_Parent->GetWorldScale();
    }
    m_ScaleDirty = false;
}

void Transform::UpdateWorldMatrix() {
    const glm::mat4 trans = glm::translate(glm::mat4(1.0f), GetWorldPosition());
    const glm::mat4 rot = glm::mat4_cast(GetWorldRotation());
    const glm::mat4 scale = glm::scale(glm::mat4(1.0f), GetWorldScale());

    m_WorldMatrix = trans * rot * scale;
    m_MatrixDirty = false;
}

void Transform::SetPositionDirty() {
    m_PositionDirty = true;
    m_MatrixDirty = true;
    for (auto child: m_Children) {
        child->SetPositionDirty();
    }
}

void Transform::SetRotationDirty() {
    m_RotationDirty = true;
    m_MatrixDirty = true;
    for(Transform* childPtr : m_Children) {
        if(not childPtr->m_RotationDirty) {
            childPtr->SetRotationDirty();
        }
    }
}

void Transform::SetScaleDirty() {
    m_ScaleDirty = true;
    m_MatrixDirty = true;

    for(Transform* childPtr : m_Children) {
        if(not childPtr->m_ScaleDirty) {
            childPtr->SetScaleDirty();
        }
    }
}


